#pragma once

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <iostream>

// Logs at CRITICAL level, flushes spdlog buffers, prints to stderr, then exits.
// Use instead of assert(false) for unrecoverable programmer errors.
// The macro form preserves __FILE__ / __LINE__ in spdlog output.
#define FATAL_ERROR(...)                                                   \
  do {                                                                     \
    SPDLOG_CRITICAL(__VA_ARGS__);                                          \
    spdlog::shutdown();                                                    \
    std::cerr << "\nFATAL: " << fmt::format(__VA_ARGS__) << "\n";         \
    std::exit(EXIT_FAILURE);                                               \
  } while (0)
