#include "rdb/error.hpp"

#include <atomic>
#include <cstdio>
#include <cstdlib>

#include <spdlog/spdlog.h>

namespace rdb {

namespace {

// Handler fatal() to polityka PROCESU, jak std::set_terminate: ustawia ja host (demon w
// launcher.cpp), nie silnik, i nie jest stanem zadnej instancji Engine. Stad jedyna zmienna o
// zasiegu pliku w bibliotece rdb poza sklepem MEMORY - wymieniona w test/embedding_boundary.py.
std::atomic<FatalHandler> &fatalHandlerSlot() noexcept {
  static std::atomic<FatalHandler> slot{nullptr};
  return slot;
}

[[noreturn]] void defaultFatalHandler(std::string_view message, const std::source_location &where) noexcept {
  // Logowanie nie moze tu zawiesc procesu w inny sposob niz zamierzony: bez loggera zostaje
  // stderr. Flush, nie shutdown - patrz komentarz przy FatalError w fatalError.hpp.
  if (auto *logger = spdlog::default_logger_raw(); logger != nullptr) {
    logger->log(spdlog::source_loc{where.file_name(), static_cast<int>(where.line()), where.function_name()},
                spdlog::level::critical, "{}", message);
    logger->flush();
  }
  std::fprintf(stderr, "\nFATAL: %.*s\n", static_cast<int>(message.size()), message.data());
  std::fflush(stderr);
  std::abort();
}

}  // namespace

void fatal(std::string_view message, std::source_location where) noexcept {
  const FatalHandler handler = fatalHandlerSlot().load(std::memory_order_acquire);
  if (handler != nullptr) handler(message, where);
  // Handler, ktory wrocil, jest bledem hosta - konczymy tak, jak konczylby domyslny.
  defaultFatalHandler(message, where);
}

FatalHandler setFatalHandler(FatalHandler handler) noexcept {
  return fatalHandlerSlot().exchange(handler, std::memory_order_acq_rel);
}

}  // namespace rdb
