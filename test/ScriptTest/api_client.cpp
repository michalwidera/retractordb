#include <signal.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "retractordb/client.hpp"

using namespace std::chrono_literals;
using retractordb::Error;

static_assert(std::is_nothrow_move_constructible_v<retractordb::Client>);
static_assert(std::is_nothrow_move_assignable_v<retractordb::Client>);
static_assert(!std::is_copy_constructible_v<retractordb::Client>);
static_assert(!std::is_copy_assignable_v<retractordb::Client>);
static_assert(std::is_same_v<decltype(std::declval<retractordb::Client &>().ping()), void>);

void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}

template <typename F>
void expectError(const std::string &code, F action) {
  try {
    action();
  } catch (const Error &error) {
    require(error.code == code, error.what());
    return;
  }
  throw std::runtime_error("Expected error: " + code);
}

retractordb::Client makeClient(const std::string &server, const std::string &xqry) {
  retractordb::Client client(server, {.xqry = xqry, .timeout = 2s});
  return client;
}

void checkMoves(const std::string &server, const std::string &xqry, const std::string &stream) {
  auto original = makeClient(server, xqry);
  auto samples  = original.subscribe(stream);
  const int pid = samples.pid();
  {
    std::vector<retractordb::Client> clients;
    clients.reserve(1);
    clients.push_back(std::move(original));
    original.close();
    clients.push_back(makeClient(server, xqry));
    clients.front().ping();
    {
      auto ping = [client = std::move(clients.front())]() mutable { client.ping(); };
      clients.front().close();
      ping();
      require(kill(pid, 0) == 0, "move closed transferred subscription");
    }
    require(kill(pid, 0) == -1, "lambda destruction left child");
    require(!samples.next(), "destroyed moved client still reads");
  }

  auto source           = makeClient(server, xqry);
  auto incoming         = source.subscribe(stream);
  const int incomingPid = incoming.pid();
  {
    auto target           = makeClient(server, xqry);
    auto replaced         = target.subscribe(stream);
    const int replacedPid = replaced.pid();
    target                = std::move(source);
    source.close();
    require(kill(replacedPid, 0) == -1, "move assignment left replaced child");
    require(!replaced.next(), "replaced client still reads");
    target.ping();
    require(kill(incomingPid, 0) == 0, "move assignment closed transferred subscription");
    target.close();
    require(kill(incomingPid, 0) == -1, "moved client.close left child");
    require(!incoming.next(), "closed moved client still reads");
    target.close();
    expectError("closed", [&] { target.ping(); });
  }
  source = makeClient(server, xqry);
  source.ping();
}

int main(int argc, char **argv) {
  if (argc != 4) return 2;
  try {
    const bool fake = std::string(argv[3]) == "fake";
    checkMoves(argv[1], argv[2], fake ? "wait" : "numbers");
    retractordb::Client db(argv[1], {.xqry = argv[2], .timeout = 2s});
    db.ping();
    require(!db.streams().empty(), "streams");
    const auto schema = db.describe(fake ? "valid" : "numbers");
    require(!schema.fields.empty(), "schema");
    if (fake) {
      for (const auto &[name, code] : {std::pair{"badping", "protocol_error"}, {"pingerror", "server_no_response"}}) {
        auto broken = makeClient(name, argv[2]);
        expectError(code, [&] { broken.ping(); });
      }
      {
        auto samples  = db.subscribe("valid");
        const int pid = samples.pid();
        for (int i = 0; i < 3; ++i) {
          const auto row = samples.next(2s);
          require(row.has_value(), "missing record");
          require(std::get<std::int64_t>(row->values.at("a")[0]) == 10, "array[0]");
          require(std::holds_alternative<std::monostate>(row->values.at("a")[1]), "array NULL");
          require(std::get<std::string>(row->values.at("s")[0]) == "null\n\"\\ world", "escaped string");
          require(std::get<retractordb::Rational>(row->values.at("r")[0]) == retractordb::Rational{1, 3}, "rational");
        }
        require(!samples.next(2s), "missing end");
        require(samples.endReason() == "limit", "end reason");
        require(kill(pid, 0) == -1, "child not reaped");
      }
      expectError("protocol_error", [&] { return db.subscribe("badversion"); });
      for (const auto &[mode, code] :
           {std::pair{"bad", "protocol_error"}, {"exit", "process_exit"}, {"error", "stream_not_found"}}) {
        expectError(code, [&] {
          auto samples = db.subscribe(mode);
          return samples.next(2s);
        });
      }
      for (const std::string broken : {"nokey", "badtype", "baddelta", "hugedelta", "zerodelta"}) {
        retractordb::Client client(broken, {.xqry = argv[2], .timeout = 2s});
        expectError("protocol_error", [&] { return client.streams(); });
        expectError("protocol_error", [&] { return db.describe(broken); });
        expectError("protocol_error", [&] { return db.subscribe(broken); });
      }
      {
        auto samples = db.subscribe("wait");
        expectError("read_timeout", [&] { return samples.next(20ms); });
        const auto begin = std::chrono::steady_clock::now();
        const int pid    = samples.pid();
        samples.close();
        require(std::chrono::steady_clock::now() - begin < 1600ms, "close timeout");
        require(kill(pid, 0) == -1, "stubborn child not reaped");
      }
      expectError("buffer_overflow", [&] {
        auto samples = db.subscribe("flood", {.capacity = 2});
        std::this_thread::sleep_for(300ms);
        return samples.next(2s);
      });
    } else {
      auto a = db.subscribe("numbers", {.limit = 3});
      auto b = db.subscribe("copy", {.limit = 3});
      require(a.pid() != b.pid(), "subscriptions share a PID");
      for (auto *samples : {&a, &b}) {
        for (int i = 0; i < 3; ++i) {
          const auto row = samples->next(2s);
          require(row.has_value(), "missing array record");
          std::vector<std::int64_t> items;
          for (const auto &field : samples->schema().fields)
            for (const auto &item : row->values.at(field.name))
              items.push_back(std::get<std::int64_t>(item));
          require(items.size() == 3, "array truncated");
          for (int n = 0; n < 3; ++n)
            require(items[n] == 10 + n, "array value");
        }
        require(!samples->next(2s), "limit not honored");
      }
      auto ratios    = db.subscribe("ratios", {.limit = 1});
      const auto row = ratios.next(2s);
      require(row.has_value(), "missing rational record");
      require(std::get<retractordb::Rational>(row->values.at(ratios.schema().fields[0].name)[0]) == retractordb::Rational{10, 3},
              "rational value");
      require(std::holds_alternative<std::monostate>(row->values.at(ratios.schema().fields[1].name)[0]), "NULL value");
      expectError("stream_not_found", [&] { return db.subscribe("missing"); });
      db.ping();
    }
    auto remaining = db.subscribe(fake ? "wait" : "numbers");
    const int pid  = remaining.pid();
    db.close();
    require(kill(pid, 0) == -1, "client.close left child");
    require(!remaining.next(), "closed subscription still reads");
    expectError("closed", [&] { db.ping(); });
    std::cout << "PASS C++ " << argv[3] << '\n';
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
