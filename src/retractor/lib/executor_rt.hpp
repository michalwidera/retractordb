#pragma once

#ifdef __linux__

#include <time.h>

bool rtCheckAndPrint();
bool rtActivate();
void rtAbsoluteSleep(const struct timespec &anchor, long interval_ms);

#endif  // __linux__
