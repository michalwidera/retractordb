#pragma once

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <iostream>
#include <source_location>

// [[noreturn]] replacement for the old FATAL_ERROR macro.
// The struct+CTAD idiom allows std::source_location as a defaulted trailing
// parameter alongside a variadic template — impossible with a plain function.
// Format string is checked at compile time via fmt::format_string<Args...>.
template <typename... Args>
struct FatalError {
  [[noreturn]] FatalError(fmt::format_string<Args...> fmt_str, Args &&...args,
                          std::source_location loc = std::source_location::current()) {
    auto msg = fmt::format(fmt_str, std::forward<Args>(args)...);
    spdlog::default_logger_raw()->log(spdlog::source_loc{loc.file_name(), static_cast<int>(loc.line()), loc.function_name()},
                                      spdlog::level::critical, msg);
    spdlog::shutdown();
    std::cerr << "\nFATAL: " << msg << "\n";
    std::exit(EXIT_FAILURE);
  }
};

template <typename... Args>
FatalError(fmt::format_string<Args...>, Args &&...) -> FatalError<Args...>;
