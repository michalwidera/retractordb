#pragma once

#include "QStruct.h"  // token, std::list
#include "rdb/payload.h"

class expressionEvaluator {
 private:
  /* data */
 public:
  expressionEvaluator(/* args */);
  ~expressionEvaluator();

  rdb::descFldVT eval(std::list<token> program, rdb::payload *payload = nullptr);
};
