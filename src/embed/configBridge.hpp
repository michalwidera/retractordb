#pragma once

#include <string>

#include "rdb/error.hpp"
#include "retractor/lib/appConfig.hpp"

namespace rdb::embed {
[[nodiscard]] Result<AppConfig> embeddedConfig(const std::string &path);
}
