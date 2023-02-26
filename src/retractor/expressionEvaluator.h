#pragma once

#include <any>
#include <boost/rational.hpp>
#include <stack>
#include <string>
#include <variant>

#include "QStruct.h"  // token

#define ENUMDECL_H_CREATE_VARIANT_T
#include "fldType.h"  // rdb::variant_t

// typedef std::variant<int, unsigned, std::string> variant_t;
class expressionEvaluator {
 private:
  /* data */
 public:
  expressionEvaluator(/* args */);
  ~expressionEvaluator();

  rdb::variant_t eval(std::list<token> program);
};
