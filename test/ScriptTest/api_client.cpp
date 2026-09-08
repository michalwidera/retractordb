#include <signal.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#include "retractordb/client.hpp"

using namespace std::chrono_literals;
using retractordb::Error;

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

int main(int argc, char **argv) {
  if (argc != 4) return 2;
  try {
    const bool fake = std::string(argv[3]) == "fake";
    retractordb::Client db(argv[1], {.xqry = argv[2], .timeout = 2s});
    require(db.ping(), "ping");
    require(!db.streams().empty(), "streams");
    const auto schema = db.describe(fake ? "valid" : "numbers");
    require(!schema.fields.empty(), "schema");
    if (fake) {
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
      expectError("protocol_error", [&] { db.subscribe("badversion"); });
      for (const auto &[mode, code] :
           {std::pair{"bad", "protocol_error"}, {"exit", "process_exit"}, {"error", "stream_not_found"}}) {
        expectError(code, [&] {
          auto samples = db.subscribe(mode);
          samples.next(2s);
        });
      }
      {
        auto samples = db.subscribe("wait");
        expectError("read_timeout", [&] { samples.next(20ms); });
        const auto begin = std::chrono::steady_clock::now();
        const int pid    = samples.pid();
        samples.close();
        require(std::chrono::steady_clock::now() - begin < 1600ms, "close timeout");
        require(kill(pid, 0) == -1, "stubborn child not reaped");
      }
      expectError("buffer_overflow", [&] {
        auto samples = db.subscribe("flood", {.capacity = 2});
        std::this_thread::sleep_for(300ms);
        samples.next(2s);
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
      expectError("stream_not_found", [&] { db.subscribe("missing"); });
      require(db.ping(), "closing clients stopped server");
    }
    auto remaining = db.subscribe(fake ? "wait" : "numbers");
    const int pid  = remaining.pid();
    db.close();
    require(kill(pid, 0) == -1, "client.close left child");
    require(!remaining.next(), "closed subscription still reads");
    std::cout << "PASS C++ " << argv[3] << '\n';
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
