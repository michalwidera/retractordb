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

/// @requirements-exempt Error: Nosnik kodu i komunikatu bledu klienta, bez wlasnej logiki obslugi bledu.
class Error : public std::runtime_error {
 public:
  std::string code;
  Error(std::string code, const std::string &message);
};

/// @requirements-exempt Rational: Para licznika i mianownika; porownanie wartosci jest domyslne.
struct Rational {
  std::int64_t numerator;
  std::int64_t denominator;
  bool operator==(const Rational &) const = default;
};

using Value = std::variant<std::monostate, std::int64_t, double, Rational, std::pair<std::int64_t, std::int64_t>,
                           std::pair<std::string, std::int64_t>, std::string>;

/// @requirements-exempt Stream: Opis nazwy i interwalu strumienia zwracany przez klienta.
struct Stream {
  std::string name;
  Rational delta;
};

/// @requirements-exempt Field: Opis nazwy, typu i krotnosci pola schematu.
struct Field {
  std::string name;
  std::string type;
  std::size_t count;
};

/// @requirements-exempt Schema: Dane schematu strumienia zwracane przez klienta.
struct Schema {
  std::string stream;
  Rational delta;
  std::string query;
  std::vector<Field> fields;
};

/// @requirements-exempt Record: Rekord danych odebrany przez klienta; bez logiki transmisji.
struct Record {
  std::string stream;
  // Skalar ma jeden element, tablica zachowuje kolejnosc i NULL kazdego elementu.
  std::map<std::string, std::vector<Value>> values;
};

/// @requirements-exempt Options: Parametry uruchamiania i czasu oczekiwania klienta.
struct Options {
  std::string xqry{"xqry"};
  std::chrono::milliseconds timeout{5000};
};

/// @requirements-exempt SubscribeOptions: Parametry limitu rekordow, czasu bezczynnosci i pojemnosci subskrypcji.
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

  [[nodiscard]] const Schema &schema() const;
  [[nodiscard]] std::optional<Record> next(std::optional<std::chrono::milliseconds> timeout = std::nullopt);
  void close() noexcept;
  [[nodiscard]] int pid() const;
  [[nodiscard]] std::string endReason() const;

 private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
  explicit Subscription(std::shared_ptr<Impl> impl);
  friend class Client;
};

class Client {
 public:
  explicit Client(std::string server, Options options = {});
  Client(Client &&) noexcept;
  Client &operator=(Client &&) noexcept;
  ~Client();
  Client(const Client &)            = delete;
  Client &operator=(const Client &) = delete;

  void ping();
  [[nodiscard]] std::vector<Stream> streams();
  [[nodiscard]] Schema describe(const std::string &stream);
  [[nodiscard]] Subscription subscribe(const std::string &stream, SubscribeOptions options = {});
  void close() noexcept;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}  // namespace retractordb
