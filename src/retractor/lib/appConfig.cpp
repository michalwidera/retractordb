#include "appConfig.hpp"

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <spdlog/spdlog.h>
#include <toml++/toml.hpp>

namespace {

constexpr int kWarnHighIpcQueueBufferSeconds{3600};
constexpr int kWarnHighIpcMinQueueElements{1'000'000};
constexpr int kWarnHighIpcClientResponseMaxFails{1000};
constexpr int kWarnHighTimingServerStartupWaitSeconds{3600};
constexpr int kWarnHighTimingServerStartupPollIntervalMs{10'000};
constexpr int kWarnHighTimingQueryNoDataTimeoutMs{600'000};
constexpr int kWarnHighSchedulingRtPriority{80};

// Normalizuje katalog storage: niepusty bez końcowego '/' dostaje '/'
// — spójnie z konwencją dyrektywy :STORAGE w RQLParser.
std::string normalizeStorageDir(std::string dir) {
  if (!dir.empty() && dir.back() != '/') dir.push_back('/');
  return dir;
}

void sanitizeConfig(AppConfig &cfg) {
  const AppConfig defaults{};

  if (cfg.ipcQueueBufferSeconds <= 0) {
    SPDLOG_WARN("Invalid config ipc.queue_buffer_seconds={} (must be > 0). Using default {}.", cfg.ipcQueueBufferSeconds,
                defaults.ipcQueueBufferSeconds);
    cfg.ipcQueueBufferSeconds = defaults.ipcQueueBufferSeconds;
  } else if (cfg.ipcQueueBufferSeconds > kWarnHighIpcQueueBufferSeconds) {
    SPDLOG_WARN("Suspiciously high ipc.queue_buffer_seconds={} (1h+ queue headroom).", cfg.ipcQueueBufferSeconds);
  }

  if (cfg.ipcMinQueueElements <= 0) {
    SPDLOG_WARN("Invalid config ipc.min_queue_elements={} (must be > 0). Using default {}.", cfg.ipcMinQueueElements,
                defaults.ipcMinQueueElements);
    cfg.ipcMinQueueElements = defaults.ipcMinQueueElements;
  } else if (cfg.ipcMinQueueElements > kWarnHighIpcMinQueueElements) {
    SPDLOG_WARN("Suspiciously high ipc.min_queue_elements={} (may consume significant memory).", cfg.ipcMinQueueElements);
  }

  if (cfg.ipcClientResponseMaxFails <= 0) {
    SPDLOG_WARN("Invalid config ipc.client_response_max_fails={} (must be > 0). Using default {}.",
                cfg.ipcClientResponseMaxFails, defaults.ipcClientResponseMaxFails);
    cfg.ipcClientResponseMaxFails = defaults.ipcClientResponseMaxFails;
  } else if (cfg.ipcClientResponseMaxFails > kWarnHighIpcClientResponseMaxFails) {
    SPDLOG_WARN("Suspiciously high ipc.client_response_max_fails={} (long request wait).", cfg.ipcClientResponseMaxFails);
  }

  if (cfg.timingServerStartupWaitSeconds <= 0) {
    SPDLOG_WARN("Invalid config timing.server_startup_wait_s={} (must be > 0). Using default {}.",
                cfg.timingServerStartupWaitSeconds, defaults.timingServerStartupWaitSeconds);
    cfg.timingServerStartupWaitSeconds = defaults.timingServerStartupWaitSeconds;
  } else if (cfg.timingServerStartupWaitSeconds > kWarnHighTimingServerStartupWaitSeconds) {
    SPDLOG_WARN("Suspiciously high timing.server_startup_wait_s={} (1h+ startup wait).", cfg.timingServerStartupWaitSeconds);
  }

  if (cfg.timingServerStartupPollIntervalMs <= 0) {
    SPDLOG_WARN("Invalid config timing.server_startup_poll_ms={} (must be > 0). Using default {}.",
                cfg.timingServerStartupPollIntervalMs, defaults.timingServerStartupPollIntervalMs);
    cfg.timingServerStartupPollIntervalMs = defaults.timingServerStartupPollIntervalMs;
  } else if (cfg.timingServerStartupPollIntervalMs > kWarnHighTimingServerStartupPollIntervalMs) {
    SPDLOG_WARN("Suspiciously high timing.server_startup_poll_ms={} (slow startup detection).",
                cfg.timingServerStartupPollIntervalMs);
  }

  if (cfg.timingQueryNoDataTimeoutMs <= 0) {
    SPDLOG_WARN("Invalid config timing.query_no_data_timeout_ms={} (must be > 0). Using default {}.",
                cfg.timingQueryNoDataTimeoutMs, defaults.timingQueryNoDataTimeoutMs);
    cfg.timingQueryNoDataTimeoutMs = defaults.timingQueryNoDataTimeoutMs;
  } else if (cfg.timingQueryNoDataTimeoutMs > kWarnHighTimingQueryNoDataTimeoutMs) {
    SPDLOG_WARN("Suspiciously high timing.query_no_data_timeout_ms={} (10m+ no-data timeout).", cfg.timingQueryNoDataTimeoutMs);
  }

  if (cfg.schedulingRtPriority < appcfg::kRtPriorityMin || cfg.schedulingRtPriority > appcfg::kRtPriorityMax) {
    SPDLOG_WARN("Invalid config scheduling.rt_priority={} (allowed range 1..99). Using default {}.", cfg.schedulingRtPriority,
                defaults.schedulingRtPriority);
    cfg.schedulingRtPriority = defaults.schedulingRtPriority;
  } else if (cfg.schedulingRtPriority > kWarnHighSchedulingRtPriority) {
    SPDLOG_WARN("High scheduling.rt_priority={} (may starve lower-priority tasks).", cfg.schedulingRtPriority);
  }

  if (!cfg.lockDir.empty() && !std::filesystem::path(cfg.lockDir).is_absolute()) {
    SPDLOG_WARN("paths.lock_dir='{}' is not an absolute path.", cfg.lockDir);
  }
}

// Nakłada ustawienia z jednej tabeli TOML na akumulowaną konfigurację.
// Klucze nieobecne w tabeli pozostawiają dotychczasową wartość (warstwowość).
void applyTable(const toml::table &tbl, AppConfig &cfg) {
  if (auto v = tbl.at_path("storage.dir").value<std::string>(); v) cfg.storageDir = *v;

  if (auto v = tbl.at_path("ipc.queue_buffer_seconds").value<int>(); v) cfg.ipcQueueBufferSeconds = *v;
  if (auto v = tbl.at_path("ipc.min_queue_elements").value<int>(); v) cfg.ipcMinQueueElements = *v;
  if (auto v = tbl.at_path("ipc.client_response_max_fails").value<int>(); v) cfg.ipcClientResponseMaxFails = *v;

  if (auto v = tbl.at_path("timing.server_startup_wait_s").value<int>(); v) cfg.timingServerStartupWaitSeconds = *v;
  if (auto v = tbl.at_path("timing.server_startup_poll_ms").value<int>(); v) cfg.timingServerStartupPollIntervalMs = *v;
  if (auto v = tbl.at_path("timing.query_no_data_timeout_ms").value<int>(); v) cfg.timingQueryNoDataTimeoutMs = *v;

  if (auto v = tbl.at_path("scheduling.rt_priority").value<int>(); v) cfg.schedulingRtPriority = *v;

  if (auto v = tbl.at_path("paths.lock_dir").value<std::string>(); v) cfg.lockDir = *v;

  if (auto v = tbl.at_path("service.query_file").value<std::string>(); v) cfg.serviceQueryFile = *v;
}

// Ścieżka pliku konfiguracyjnego użytkownika wg XDG ($XDG_CONFIG_HOME lub ~/.config).
std::optional<std::filesystem::path> userConfigPath() {
  if (const char *xdg = std::getenv("XDG_CONFIG_HOME"); xdg != nullptr && xdg[0] != '\0')
    return std::filesystem::path(xdg) / "retractor" / "retractor.toml";
  if (const char *home = std::getenv("HOME"); home != nullptr && home[0] != '\0')
    return std::filesystem::path(home) / ".config" / "retractor" / "retractor.toml";
  return std::nullopt;
}

}  // namespace

AppConfig loadAppConfig(const std::optional<std::string> &cliPath) {
  AppConfig cfg;

  if (cliPath) {
    // Jawnie podana ścieżka: plik musi istnieć i być poprawny — błąd jest twardy
    // (toml::parse_file rzuca toml::parse_error). Wywołujący raportuje go użytkownikowi.
    const toml::table tbl = toml::parse_file(*cliPath);
    applyTable(tbl, cfg);
    cfg.loadedFrom.push_back(*cliPath);
    sanitizeConfig(cfg);
    cfg.storageDir = normalizeStorageDir(cfg.storageDir);
    return cfg;
  }

  // Wyszukiwanie warstwowe: system → użytkownik (kolejne nadpisują klucze).
  std::vector<std::filesystem::path> candidates{"/etc/retractor/retractor.toml"};
  if (auto up = userConfigPath(); up) candidates.push_back(*up);

  for (const auto &path : candidates) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) continue;  // brak pliku = stan poprawny
    try {
      const toml::table tbl = toml::parse_file(path.string());
      applyTable(tbl, cfg);
      cfg.loadedFrom.push_back(path.string());
    } catch (const toml::parse_error &e) {
      SPDLOG_WARN("Config parse error in {}: {} — skipping this layer", path.string(), e.what());
    }
  }

  sanitizeConfig(cfg);
  cfg.storageDir = normalizeStorageDir(cfg.storageDir);
  return cfg;
}
