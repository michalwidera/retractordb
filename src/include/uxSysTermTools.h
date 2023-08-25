#pragma once

#include <string>

int _kbhit(void);
int _getch();
void fixArgcv(int argc, char *argv[]);
std::string setupLoggerMain(const std::string &loggerFile);