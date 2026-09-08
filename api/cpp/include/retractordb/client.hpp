#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace retractordb {

class Error : public std::runtime_error {
 public:
  std::string code;
  Error(std::string code, const std::string &message);
};

struct Rational {
  std::int64_t numerator;
  std::int64_t denominator;
  bool operator==(const Rational &) const = default;
};

using Value = std::variant<std::monostate, std::int64_t, double, Rational, std::pair<std::int64_t, std::int64_t>,
                           std::pair<std::string, std::int64_t>, std::string>;

struct Stream {
  std::string name;
  Rational delta;
};

struct Field {
  std::string name;
  std::string type;
  std::size_t count;
};

struct Schema {
  std::string stream;
  Rational delta;
  std::string query;
  std::vector<Field> fields;
};

struct Record {
  std::string stream;
  // Skalar ma jeden element, tablica zachowuje kolejnosc i NULL kazdego elementu.
  std::map<std::string, std::vector<Value>> values;
};

struct Options {
  std::string xqry{"xqry"};
  std::chrono::milliseconds timeout{5000};
};

struct SubscribeOptions {
  int limit{0};
  std::chrono::milliseconds idleTimeout{0};
  std::size_t capacity{1024};
};

class Client;

class Subscription {
 public:
  Subscription(Subscription &&) noexcept;
  Subscription &operator=(Subscription &&) noexcept;
  ~Subscription();
  Subscription(const Subscription &)            = delete;
  Subscription &operator=(const Subscription &) = delete;

  const Schema &schema() const;
  std::optional<Record> next(std::optional<std::chrono::milliseconds> timeout = std::nullopt);
  void close() noexcept;
  int pid() const;
  std::string endReason() const;

 private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
  explicit Subscription(std::shared_ptr<Impl> impl);
  friend class Client;
};

class Client {
 public:
  explicit Client(std::string server, Options options = {});
  ~Client();
  Client(const Client &)            = delete;
  Client &operator=(const Client &) = delete;

  bool ping();
  std::vector<Stream> streams();
  Schema describe(const std::string &stream);
  Subscription subscribe(const std::string &stream, SubscribeOptions options = {});
  void close() noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace retractordb
