#pragma once
#include "rdb/storageacc.hpp"
#include "xtrdbTypes.hpp"

void executeSetPos(rdb::storage& dacc, payloadStatusType& payloadStatus);
void executeGetPos(rdb::storage& dacc);
