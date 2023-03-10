#pragma once

#include "QStruct.h"  // token, std::list

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
