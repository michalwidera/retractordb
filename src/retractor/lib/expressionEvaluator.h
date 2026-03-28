#pragma once

#include "rdb/payload.h"
#include "token.h"  // token, std::list

class expressionEvaluator {
 private:
  /* data */
 public:
  expressionEvaluator(/* args */);
  ~expressionEvaluator();

  rdb::descFldVT eval(std::list<token> program, rdb::payload *payload = nullptr);
};
