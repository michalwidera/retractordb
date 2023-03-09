#pragma once

#include <any>
#include <boost/rational.hpp>
#include <stack>
#include <string>
#include <variant>

#include "QStruct.h"  // token

#define FLDTYPE_H_CREATE_VARIANT_T
#include "fldType.h"  // rdb::descFldVT

class expressionEvaluator {
 private:
  /* data */
 public:
  expressionEvaluator(/* args */);
  ~expressionEvaluator();

  rdb::descFldVT eval(std::list<token> program);
};
