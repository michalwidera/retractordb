#include "retractordb/client.hpp"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <set>
#include <thread>

#include <boost/json.hpp>

extern char **environ;

namespace retractordb {
namespace {
namespace json     = boost::json;
using Clock        = std::chrono::steady_clock;
using Milliseconds = std::chrono::milliseconds;

std::string string(const json::value &value) { return std::string(value.as_string()); }

std::int64_t integer(const std::string &text) {
  std::size_t end;
  const auto value = std::stoll(text, &end);
  if (end != text.size()) throw Error("protocol_error", "Invalid integer");
  return value;
}

Rational rational(const std::string &text) {
  const auto slash = text.find('/');
  Rational value{integer(text.substr(0, slash)), slash == std::string::npos ? 1 : integer(text.substr(slash + 1))};
  if (value.denominator == 0) throw Error("protocol_error", "Zero rational denominator");
  return value;
}

Schema schema(const json::object &event) {
  Schema result{string(event.at("stream")), rational(string(event.at("delta"))), string(event.at("query")), {}};
  std::set<std::string> names;
  for (const auto &item : event.at("fields").as_array()) {
    const auto &field = item.as_object();
    const auto count  = field.at("count").as_int64();
    const auto name   = string(field.at("name"));
    if (count < 1 || count > 1048576 || !names.insert(name).second) throw Error("protocol_error", "Invalid schema fields");
    result.fields.push_back({name, string(field.at("type")), static_cast<std::size_t>(count)});
  }
  return result;
}

Value value(const json::value &raw, const std::string &type) {
  if (raw.is_null()) return std::monostate{};
  const auto text = string(raw);
  if (type == "BYTE" || type == "INTEGER" || type == "UINT") return integer(text);
  if (type == "FLOAT" || type == "DOUBLE") {
    std::size_t end;
    const double result = std::stod(text, &end);
    if (end != text.size()) throw Error("protocol_error", "Invalid floating point value");
    return result;
  }
  if (type == "RATIONAL") return rational(text);
  if (type == "INTPAIR") {
    const auto comma = text.find(',');
    if (comma == std::string::npos) throw Error("protocol_error", "Invalid integer pair");
    return std::pair{integer(text.substr(0, comma)), integer(text.substr(comma + 1))};
  }
  if (type == "IDXPAIR") {
    const auto bracket = text.rfind('[');
    if (bracket == std::string::npos || text.back() != ']') throw Error("protocol_error", "Invalid index pair");
    return std::pair{text.substr(0, bracket), integer(text.substr(bracket + 1, text.size() - bracket - 2))};
  }
  if (type == "STRING") return text;
  throw Error("protocol_error", "Unsupported field type: " + type);
}

Record record(const json::object &event, const Schema &description) {
  Record result{string(event.at("stream")), {}};
  const auto &raw    = event.at("values").as_array();
  std::size_t offset = 0;
  for (const auto &field : description.fields) {
    auto &values = result.values[field.name];
    for (std::size_t i = 0; i < field.count; ++i)
      values.push_back(value(raw.at(offset++), field.type));
  }
  if (offset != raw.size() || result.stream != description.stream) throw Error("protocol_error", "Record does not match schema");
  return result;
}

// Jeden watek czyta oba potoki. Nigdy nie czeka na miejsce w kolejce aplikacji.
class Process {
  int child_{-1};
  int originalPid_{-1};
  int status_{0};
  std::array<int, 2> pipes_{-1, -1};
  std::thread reader_;
  std::atomic<bool> stopping_{false};
  std::mutex mutex_;
  std::condition_variable changed_;
  std::deque<json::object> events_;
  std::optional<Error> error_;
  bool eof_{false};
  std::string diagnostics_;
  std::size_t capacity_;

  void pump() noexcept {
    try {
      std::array<pollfd, 2> fds{{{pipes_[0], POLLIN, 0}, {pipes_[1], POLLIN, 0}}};
      std::string pending;
      std::array<char, 8192> buffer;
      while (!stopping_ && (fds[0].fd >= 0 || fds[1].fd >= 0)) {
        if (::poll(fds.data(), fds.size(), 20) < 0) {
          if (errno == EINTR) continue;
          throw Error("io_error", std::strerror(errno));
        }
        for (std::size_t i = 0; i < fds.size(); ++i) {
          if (fds[i].fd < 0 || !fds[i].revents) continue;
          const auto count = ::read(fds[i].fd, buffer.data(), buffer.size());
          if (count < 0 && errno == EINTR) continue;
          if (count < 0) throw Error("io_error", std::strerror(errno));
          if (count == 0) {
            fds[i].fd = -1;
            if (i == 0 && !pending.empty()) throw Error("protocol_error", "Incomplete JSONL event");
            continue;
          }
          if (i == 1) {
            std::lock_guard lock(mutex_);
            diagnostics_.append(buffer.data(), count);
            if (diagnostics_.size() > 65536) diagnostics_.erase(0, diagnostics_.size() - 65536);
            continue;
          }
          pending.append(buffer.data(), count);
          std::size_t newline;
          while ((newline = pending.find('\n')) != std::string::npos) {
            if (newline > 1048576) throw Error("protocol_error", "Oversized JSONL event");
            auto item = json::parse(std::string_view(pending.data(), newline)).as_object();
            pending.erase(0, newline + 1);
            if (item.at("version").as_int64() != 1) throw Error("protocol_error", "Unsupported JSONL protocol version");
            std::lock_guard lock(mutex_);
            if (events_.size() >= capacity_)
              throw Error("buffer_overflow", "Application is not consuming stream events fast enough");
            events_.push_back(std::move(item));
            changed_.notify_all();
          }
          if (pending.size() > 1048576) throw Error("protocol_error", "Oversized JSONL event");
        }
      }
    } catch (const Error &error) {
      std::lock_guard lock(mutex_);
      error_ = error;
    } catch (const std::exception &error) {
      std::lock_guard lock(mutex_);
      error_.emplace("protocol_error", error.what());
    }
    std::lock_guard lock(mutex_);
    eof_ = true;
    changed_.notify_all();
  }

 public:
  Process(const std::vector<std::string> &args, std::size_t capacity) : capacity_(capacity) {
    if (!capacity) throw std::invalid_argument("capacity must be positive");
    int out[2]{-1, -1}, err[2]{-1, -1};
    posix_spawnattr_t attributes;
    int attrResult = posix_spawnattr_init(&attributes);
    if (attrResult) throw Error("spawn_error", std::strerror(attrResult));
    posix_spawn_file_actions_t actions;
    int result = posix_spawn_file_actions_init(&actions);
    if (result) {
      posix_spawnattr_destroy(&attributes);
      throw Error("spawn_error", std::strerror(result));
    }
    auto check = [](int code) {
      if (code) throw Error("spawn_error", std::strerror(code));
    };
    try {
      if (pipe2(out, O_CLOEXEC) || pipe2(err, O_CLOEXEC)) throw Error("spawn_error", std::strerror(errno));
      check(posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP));
      check(posix_spawnattr_setpgroup(&attributes, 0));
      check(posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0));
      check(posix_spawn_file_actions_adddup2(&actions, out[1], STDOUT_FILENO));
      check(posix_spawn_file_actions_adddup2(&actions, err[1], STDERR_FILENO));
      for (int fd : {out[0], out[1], err[0], err[1]})
        check(posix_spawn_file_actions_addclose(&actions, fd));
      std::vector<char *> argv;
      for (const auto &arg : args)
        argv.push_back(const_cast<char *>(arg.c_str()));
      argv.push_back(nullptr);
      check(posix_spawnp(&child_, argv[0], &actions, &attributes, argv.data(), environ));
      originalPid_ = child_;
      pipes_       = {out[0], err[0]};
      out[0] = err[0] = -1;
      reader_         = std::thread([this] { pump(); });
    } catch (...) {
      for (int fd : {out[0], out[1], err[0], err[1]})
        if (fd >= 0) ::close(fd);
      posix_spawnattr_destroy(&attributes);
      posix_spawn_file_actions_destroy(&actions);
      close();
      throw;
    }
    posix_spawnattr_destroy(&attributes);
    ::close(out[1]);
    ::close(err[1]);
    posix_spawn_file_actions_destroy(&actions);
  }

  ~Process() { close(); }
  int pid() const { return originalPid_; }

  json::object read(std::optional<Milliseconds> timeout) {
    if (timeout && timeout->count() < 0) throw std::invalid_argument("timeout must be nonnegative");
    std::unique_lock lock(mutex_);
    const auto ready = [this] { return error_ || !events_.empty() || eof_ || stopping_; };
    if (timeout) {
      if (!changed_.wait_for(lock, *timeout, ready)) throw Error("read_timeout", "No event within the read timeout");
    } else
      changed_.wait(lock, ready);
    if (error_) throw *error_;
    if (events_.empty()) throw Error("process_exit", "xqry closed its output: " + diagnostics_);
    auto result = std::move(events_.front());
    events_.pop_front();
    if (result.at("event") == "error") throw Error(string(result.at("code")), string(result.at("message")));
    return result;
  }

  void complete(Milliseconds timeout) {
    const auto deadline = Clock::now() + timeout;
    while (child_ > 0) {
      const int result = waitpid(child_, &status_, WNOHANG);
      if (result == child_) {
        child_ = -1;
        break;
      }
      if (result < 0 && errno != EINTR) throw Error("process_exit", std::strerror(errno));
      if (Clock::now() >= deadline) throw Error("process_timeout", "xqry did not exit after its final event");
      std::this_thread::sleep_for(Milliseconds(5));
    }
    if (!WIFEXITED(status_) || WEXITSTATUS(status_) != 0) throw Error("process_exit", "xqry exited unsuccessfully");
  }

  void close() noexcept {
    stopping_ = true;
    changed_.notify_all();
    if (child_ > 0) {
      kill(child_, SIGTERM);
      const auto deadline = Clock::now() + Milliseconds(1000);
      while (true) {
        const int result = waitpid(child_, &status_, WNOHANG);
        if (result == child_ || (result < 0 && errno == ECHILD)) break;
        if (Clock::now() >= deadline) {
          kill(child_, SIGKILL);
          while (waitpid(child_, &status_, 0) < 0 && errno == EINTR) {}
          break;
        }
        std::this_thread::sleep_for(Milliseconds(5));
      }
      child_ = -1;
    }
    if (reader_.joinable()) reader_.join();
    for (int &fd : pipes_) {
      if (fd >= 0) ::close(fd);
      fd = -1;
    }
  }
};
}  // namespace

Error::Error(std::string value, const std::string &message) : std::runtime_error(message), code(std::move(value)) {}

struct Subscription::Impl {
  Process process;
  Schema description;
  Milliseconds timeout;
  std::string reason;
  bool closed{false};
  Impl(const std::vector<std::string> &args, const SubscribeOptions &options, Milliseconds wait)
      : process(args, options.capacity),
        timeout(wait) {
    const auto event = process.read(timeout);
    if (event.at("event") != "schema") throw Error("protocol_error", "Expected schema event");
    description = retractordb::schema(event);
  }
  void close() noexcept {
    closed = true;
    process.close();
  }
};

Subscription::Subscription(std::shared_ptr<Impl> impl) : impl_(std::move(impl)) {}
Subscription::Subscription(Subscription &&) noexcept            = default;
Subscription &Subscription::operator=(Subscription &&) noexcept = default;
Subscription::~Subscription() { close(); }
const Schema &Subscription::schema() const { return impl_->description; }
int Subscription::pid() const { return impl_->process.pid(); }
std::string Subscription::endReason() const { return impl_->reason; }
void Subscription::close() noexcept {
  if (impl_) impl_->close();
}

std::optional<Record> Subscription::next(std::optional<Milliseconds> timeout) {
  if (!impl_ || impl_->closed) return std::nullopt;
  try {
    const auto event = impl_->process.read(timeout);
    if (event.at("event") == "end") {
      impl_->reason = string(event.at("reason"));
      impl_->process.complete(impl_->timeout);
      close();
      return std::nullopt;
    }
    if (event.at("event") != "record") throw Error("protocol_error", "Expected record or end event");
    return record(event, impl_->description);
  } catch (const Error &error) {
    if (error.code != "read_timeout") close();
    throw;
  } catch (const std::exception &error) {
    close();
    throw Error("protocol_error", error.what());
  }
}

struct Client::Impl {
  std::string server;
  Options options;
  std::vector<std::weak_ptr<Subscription::Impl>> subscriptions;
  bool closed{false};
  std::vector<std::string> args(std::initializer_list<std::string> command) const {
    if (closed) throw Error("closed", "Client is closed");
    std::vector<std::string> result{options.xqry, "--server", server, "--jsonl"};
    result.insert(result.end(), command);
    return result;
  }
  json::object command(std::string_view expected, std::initializer_list<std::string> command) const {
    try {
      Process process(args(command), 16);
      auto result = process.read(options.timeout);
      if (string(result.at("event")) != expected) throw Error("protocol_error", "Unexpected command response");
      process.complete(options.timeout);
      return result;
    } catch (const Error &) {
      throw;
    } catch (const std::exception &error) {
      throw Error("protocol_error", error.what());
    }
  }
};

Client::Client(std::string server, Options options) : impl_(std::make_unique<Impl>()) {
  if (server.empty() || server.size() > 32 || server.front() < 'a' || server.front() > 'z' ||
      server.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-") != std::string::npos)
    throw std::invalid_argument("server must be an explicit xretractor instance name");
  if (options.timeout.count() <= 0) throw std::invalid_argument("timeout must be positive");
  impl_->server  = std::move(server);
  impl_->options = std::move(options);
}
Client::~Client() { close(); }
void Client::close() noexcept {
  impl_->closed = true;
  for (const auto &weak : impl_->subscriptions)
    if (auto subscription = weak.lock()) subscription->close();
  impl_->subscriptions.clear();
}
bool Client::ping() {
  impl_->command("pong", {"--hello"});
  return true;
}
std::vector<Stream> Client::streams() {
  const auto result = impl_->command("streams", {"--dir"});
  std::vector<Stream> streams;
  for (const auto &item : result.at("streams").as_array()) {
    const auto &stream = item.as_object();
    streams.push_back({string(stream.at("name")), rational(string(stream.at("delta")))});
  }
  return streams;
}
Schema Client::describe(const std::string &stream) { return schema(impl_->command("schema", {"--detail", stream})); }
Subscription Client::subscribe(const std::string &stream, SubscribeOptions options) {
  if (options.limit < 0 || options.idleTimeout.count() < 0) throw std::invalid_argument("limits must be nonnegative");
  auto subscription =
      std::make_shared<Subscription::Impl>(impl_->args({"--select", stream, "--elimitqry", std::to_string(options.limit),
                                                        "--idle-timeout", std::to_string(options.idleTimeout.count())}),
                                           options, impl_->options.timeout);
  if (subscription->description.stream != stream) throw Error("protocol_error", "Unexpected schema stream");
  std::erase_if(impl_->subscriptions, [](const auto &weak) { return weak.expired(); });
  impl_->subscriptions.push_back(subscription);
  return Subscription(std::move(subscription));
}
}  // namespace retractordb
