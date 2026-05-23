#pragma once
#include <string>

#include "rdb/storageacc.hpp"
#include "xtrdbTypes.hpp"

void executeMeta(rdb::storage& dacc, const std::string& file, const std::string& storageParam, const Colors& colors);
void executeMetaRaw(rdb::storage& dacc, const std::string& file, const std::string& storageParam, const Colors& colors);
