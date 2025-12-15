#pragma once

#include <string>

int _kbhit(bool ignoreAnyKey);
int _getch();
void fixArgcv(int argc, char *argv[]);
std::string setupLoggerMain(const std::string &loggerFile, bool dual = false);
