#pragma once

#ifdef __linux__

#include <ctime>

#include "appConfig.hpp"

bool rtCheckAndPrint();
bool rtActivate(int priority = appcfg::kDefaultSchedulingRtPriority);
void rtAbsoluteSleep(const struct timespec &anchor, long interval_ms);

#endif  // __linux__
