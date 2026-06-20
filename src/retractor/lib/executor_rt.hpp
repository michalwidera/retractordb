#pragma once

#ifdef __linux__

#include <ctime>

bool rtCheckAndPrint();
bool rtActivate(int priority = 50);
void rtAbsoluteSleep(const struct timespec &anchor, long interval_ms);

#endif  // __linux__
