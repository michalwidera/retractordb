#pragma once

#include <any>
#include <stack>
#include <variant>

#include "QStruct.h"

typedef std::variant<std::monostate,        //
                     uint8_t,               //
                     int,                   //
                     long,                  //
                     unsigned,              //
                     float,                 //
                     double,                //
                     std::string,           //
                     boost::rational<int>>  //
    variant_t;

class expressionEvaluator {
 private:
  /* data */
 public:
  expressionEvaluator(/* args */);
  ~expressionEvaluator();

  variant_t eval(std::list<token> program);
};
