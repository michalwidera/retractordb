#pragma once

#include <any>
#include <stack>

#include "QStruct.h"

class expressionEvaluator {
 private:
  /* data */
 public:
  expressionEvaluator(/* args */);
  ~expressionEvaluator();

  std::any eval(std::list<token> program);
};
