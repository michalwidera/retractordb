#pragma once

#include <memory>
#include <sstream>
#include <string>

#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

// Na czas zycia obiektu podmienia domyslny logger spdlog na zapis do napisu. Sluzy testom
// komunikatow, ktore nie maja innego kanalu niz log (np. nadpisanie archiwum przy rotacji).
class LogCapture {
 public:
  LogCapture() : previous_(spdlog::default_logger()) {
    spdlog::set_default_logger(
        std::make_shared<spdlog::logger>("capture", std::make_shared<spdlog::sinks::ostream_sink_mt>(text_)));
  }
  ~LogCapture() { spdlog::set_default_logger(previous_); }
  LogCapture(const LogCapture &)            = delete;
  LogCapture &operator=(const LogCapture &) = delete;

  [[nodiscard]] std::string text() const { return text_.str(); }

 private:
  std::ostringstream text_;
  std::shared_ptr<spdlog::logger> previous_;
};
