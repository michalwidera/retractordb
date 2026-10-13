#include "appConfig.hpp"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include <spdlog/spdlog.h>
#include <toml++/toml.hpp>

#include "platformConfig.h"
#include "rdb/sizeLimits.hpp"

namespace {

constexpr int kWarnHighIpcQueueBufferSeconds{3600};
constexpr int kWarnHighIpcMinQueueElements{1'000'000};
constexpr int kWarnHighIpcClientResponseMaxFails{1000};
constexpr int kWarnHighTimingServerStartupWaitSeconds{3600};
constexpr int kWarnHighTimingServerStartupPollIntervalMs{10'000};
constexpr int kWarnHighTimingQueryNoDataTimeoutMs{600'000};
constexpr int kWarnHighSchedulingRtPriority{80};

// Normalizuje katalog storage: niepusty bez końcowego '/' dostaje '/'
// - spójnie z konwencją dyrektywy :STORAGE w RQLParser.
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

  if (cfg.historyMemoryMib <= 0) {
    SPDLOG_WARN("Invalid config limits.history_memory_mib={} (must be > 0). Using default {}.", cfg.historyMemoryMib,
                defaults.historyMemoryMib);
    cfg.historyMemoryMib = defaults.historyMemoryMib;
  }
}

// `[capacity, segments]` - kolejnosc jak w `RETENTION capacity segments`. Wartosc niepoprawna daje
// brak retencji, jak pozostale klucze w sanitizeConfig: klucz, ktory kasuje dane, nie zgaduje.
// Segmenty 0 znacza w RQL "bez limitu segmentow", czyli przeczylyby celowi klucza.
rdb::retention_t parseDefaultRetention(const toml::node_view<const toml::node> node) {
  if (const auto *arr = node.as_array(); arr != nullptr && arr->size() == 2) {
    const auto capacity = (*arr)[0].value_exact<std::int64_t>();
    const auto segments = (*arr)[1].value_exact<std::int64_t>();
    constexpr std::int64_t kMax{std::numeric_limits<int>::max()};
    if (capacity && segments && *capacity >= 1 && *segments >= 1 && *capacity <= kMax && *segments <= kMax)
      return {.segments = static_cast<rdb::segments_t>(*segments), .capacity = static_cast<rdb::capacity_t>(*capacity)};
  }
  SPDLOG_WARN(
      "Invalid config storage.default_retention (expected [capacity, segments], both integers > 0). "
      "Streams without RETENTION keep growing on disk.");
  return {.segments = 0, .capacity = 0};
}

// `[sources] timeout_s`: liczba z przedzialu [0, kMaxDeviceTimeoutSeconds]. Wartosc spoza niego nie
// wraca do domyslnej - zostaje powod odmowy, ktory launcher xretractora zamienia w blad startu.
void parseSourcesTimeout(const toml::node_view<const toml::node> node, AppConfig &cfg) {
  const auto value = node.value<double>();
  if (value && *value >= 0.0 && *value <= rdb::limits::kMaxDeviceTimeoutSeconds) {
    cfg.sourcesTimeoutSeconds = value;
    cfg.sourcesTimeoutError.clear();
    return;
  }
  cfg.sourcesTimeoutSeconds.reset();
  cfg.sourcesTimeoutError =
      std::format("sources.timeout_s must be a number of seconds from 0 to {}", rdb::limits::kMaxDeviceTimeoutSeconds);
  if (value) cfg.sourcesTimeoutError += std::format(", got {}", *value);
}

// `[storage] ref_dirs`: tablica bezwzglednych sciezek istniejacych katalogow. Wpis wzgledny
// zmienialby zakres uprawnien z katalogiem roboczym procesu, wiec jest bledem, nie ostrzezeniem.
void parseStorageRefDirs(const toml::node_view<const toml::node> node, AppConfig &cfg) {
  cfg.storageRefDirs.clear();
  cfg.storageRefDirsError.clear();
  const auto *dirs = node.as_array();
  if (dirs == nullptr) {
    cfg.storageRefDirsError = "storage.ref_dirs must be an array of absolute directory paths";
    return;
  }
  for (const auto &item : *dirs) {
    const auto dir = item.value<std::string>();
    std::error_code ec;
    if (!dir || !std::filesystem::path(*dir).is_absolute() || !std::filesystem::is_directory(*dir, ec)) {
      cfg.storageRefDirs.clear();
      cfg.storageRefDirsError = "storage.ref_dirs must be an array of absolute directory paths";
      if (dir) cfg.storageRefDirsError += std::format(", got '{}', which is not an absolute path to a directory", *dir);
      return;
    }
    cfg.storageRefDirs.push_back(*dir);
  }
}

// Nakłada ustawienia z jednej tabeli TOML na akumulowaną konfigurację.
// Klucze nieobecne w tabeli pozostawiają dotychczasową wartość (warstwowość).
void applyTable(const toml::table &tbl, AppConfig &cfg, const std::string &path) {
  if (auto v = tbl.at_path("storage.dir").value<std::string>(); v) cfg.storageDir = *v;
  if (auto v = tbl.at_path("storage.default_retention"); v) cfg.defaultRetention = parseDefaultRetention(v);
  if (auto v = tbl.at_path("storage.ref_dirs"); v) parseStorageRefDirs(v, cfg);

  if (auto v = tbl.at_path("sources.timeout_s"); v) parseSourcesTimeout(v, cfg);

  if (auto v = tbl.at_path("ipc.queue_buffer_seconds").value<int>(); v) cfg.ipcQueueBufferSeconds = *v;
  if (auto v = tbl.at_path("ipc.min_queue_elements").value<int>(); v) cfg.ipcMinQueueElements = *v;
  if (auto v = tbl.at_path("ipc.client_response_max_fails").value<int>(); v) cfg.ipcClientResponseMaxFails = *v;

  if (auto v = tbl.at_path("timing.server_startup_wait_s").value<int>(); v) cfg.timingServerStartupWaitSeconds = *v;
  if (auto v = tbl.at_path("timing.server_startup_poll_ms").value<int>(); v) cfg.timingServerStartupPollIntervalMs = *v;
  if (auto v = tbl.at_path("timing.query_no_data_timeout_ms").value<int>(); v) cfg.timingQueryNoDataTimeoutMs = *v;

  if (auto v = tbl.at_path("scheduling.rt_priority").value<int>(); v) cfg.schedulingRtPriority = *v;

  if (auto v = tbl.at_path("paths.lock_dir").value<std::string>(); v) {
    // Kazda warstwa musi byc poprawna, nawet gdy nastepna nadpisuje ten klucz.
    if (!v->empty() && !std::filesystem::path(*v).is_absolute())
      throw std::invalid_argument(
          std::format("Configuration error in '{}': paths.lock_dir='{}' must be an absolute path", path, *v));
    cfg.lockDir = *v;
  }

  if (auto v = tbl.at_path("server.autoname").value<bool>(); v) cfg.serverAutoName = *v;

  if (auto v = tbl.at_path("service.query_file").value<std::string>(); v) cfg.serviceQueryFile = *v;
  if (auto v = tbl.at_path("service.unrestricted").value<bool>(); v) cfg.serviceUnrestricted = *v;

  if (auto v = tbl.at_path("limits.history_memory_mib").value<int>(); v) cfg.historyMemoryMib = *v;
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
  const auto loadLayer = [&](const std::string &path) {
    try {
      const toml::table tbl = toml::parse_file(path);
      applyTable(tbl, cfg, path);
      cfg.loadedFrom.push_back(path);
    } catch (const toml::parse_error &e) {
      // what() biblioteki pomija sciezke; wywolujacy musi moc wskazac wadliwy plik.
      throw toml::parse_error(std::format("Configuration error in '{}': {}", path, e.what()).c_str(), e.source());
    }
  };

  if (cliPath) {
    // Jawnie podana ścieżka: plik musi istnieć i być poprawny - błąd jest twardy, jak w warstwach
    // wykrytych (loadLayer rzuca toml::parse_error z nazwą pliku, applyTable - std::invalid_argument
    // dla względnego paths.lock_dir). Wywołujący raportuje go użytkownikowi.
    loadLayer(*cliPath);
    sanitizeConfig(cfg);
    cfg.storageDir = normalizeStorageDir(cfg.storageDir);
    return cfg;
  }

  // Wyszukiwanie warstwowe: system → użytkownik (kolejne nadpisują klucze).
  //
  // Warstwa systemowa ma WIĘCEJ niż jedną ścieżkę, bo katalog konfiguracji systemowej
  // nie jest uniwersalny. Na Linuksie jest to /etc. Na macOS /etc należy do systemu
  // (chroni je SIP, a aktualizacje systemu potrafią je nadpisać) i oprogramowanie
  // spoza systemu kładzie konfigurację w prefiksie własnej instalacji: /usr/local/etc
  // dla instalacji ręcznej i /opt/homebrew/etc dla Homebrew na Apple Silicon.
  // Kolejność jest od najbardziej ogólnej do najbardziej szczegółowej, a plik, którego
  // nie ma, nie jest błędem - więc lista dłuższa niż potrzeba nic nie kosztuje.
  std::vector<std::filesystem::path> candidates{"/etc/retractor/retractor.toml"};
#if RDB_OS_DARWIN
  candidates.emplace_back("/usr/local/etc/retractor/retractor.toml");
  candidates.emplace_back("/opt/homebrew/etc/retractor/retractor.toml");
#endif
  if (auto up = userConfigPath(); up) candidates.push_back(*up);

  for (const auto &path : candidates) {
    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec);
    if (ec) throw std::filesystem::filesystem_error("Cannot inspect configuration file", path, ec);
    if (!exists) continue;  // brak pliku = stan poprawny
    loadLayer(path.string());
  }

  sanitizeConfig(cfg);
  cfg.storageDir = normalizeStorageDir(cfg.storageDir);
  return cfg;
}
