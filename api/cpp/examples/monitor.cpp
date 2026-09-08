#include <csignal>
#include <iostream>
#include <string>

#include "retractordb/client.hpp"

namespace {
volatile std::sig_atomic_t stopped = 0;
void stop(int) { stopped = 1; }
}  // namespace

int main(int argc, char **argv) {
  if (argc < 3 || argc > 5) {
    std::cerr << "Usage: rdb_monitor SERVER STREAM [LIMIT] [XQRY]\n";
    return 2;
  }
  std::signal(SIGINT, stop);
  std::signal(SIGTERM, stop);
  try {
    retractordb::Options options;
    if (argc == 5) options.xqry = argv[4];
    retractordb::Client db(argv[1], options);
    auto samples = db.subscribe(argv[2], {.limit = argc >= 4 ? std::stoi(argv[3]) : 0});
    while (!stopped) {
      std::optional<retractordb::Record> sample;
      try {
        sample = samples.next(std::chrono::milliseconds(100));
      } catch (const retractordb::Error &error) {
        if (error.code == "read_timeout") continue;
        throw;
      }
      if (!sample) break;
      std::cout << sample->stream;
      for (const auto &[name, values] : sample->values) {
        std::cout << " " << name << "=[";
        for (const auto &value : values)
          std::visit(
              [](const auto &v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::monostate>)
                  std::cout << "null";
                else if constexpr (std::is_same_v<T, retractordb::Rational>)
                  std::cout << v.numerator << '/' << v.denominator;
                else if constexpr (requires {
                                     v.first;
                                     v.second;
                                   })
                  std::cout << v.first << ',' << v.second;
                else
                  std::cout << v;
                std::cout << ' ';
              },
              value);
        std::cout << ']';
      }
      std::cout << std::endl;
    }
  } catch (const retractordb::Error &error) {
    std::cerr << error.code << ": " << error.what() << '\n';
    return 1;
  }
}
