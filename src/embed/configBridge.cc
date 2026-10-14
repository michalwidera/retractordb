#include "configBridge.hpp"

#include <exception>

namespace rdb::embed {

// Wyspa wyjatkow: toml++ zglasza nimi niepoprawna konfiguracje. Status nie przekracza
// granicy rdzenia w postaci wyjatku, takze dla nieznanego bledu biblioteki.
Result<AppConfig> embeddedConfig(const std::string &path) {
  try {
    return loadAppConfig(path);
  } catch (const std::exception &error) {
    return fail(Errc::Config, error.what());
  } catch (...) {
    return fail(Errc::Config, "cannot read embedded configuration: " + path);
  }
}

}  // namespace rdb::embed
