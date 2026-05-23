#pragma once
#include <memory>
#include <string>

#include "rdb/storageacc.hpp"
#include "xtrdbTypes.hpp"

// Returns false if caller should `continue` (error path), true if ok to print "ok".
bool executeOpen(std::unique_ptr<rdb::storage>& dacc, std::string& file, const std::string& storageParam,
                 const std::string& storagePolicy, payloadStatusType& payloadStatus, const Colors& colors);
