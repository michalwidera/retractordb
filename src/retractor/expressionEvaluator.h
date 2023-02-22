#pragma once

#include <any>
#include <boost/rational.hpp>
#include <stack>
#include <string>
#include <variant>

#include "QStruct.h"  // token

// enumDecl.h

// sequence of types here is significatn
// it is used to promote in math operations.
typedef std::variant<std::monostate,        // BAD
                     uint8_t,               // BYTE
                     int,                   // INTEGER
                     unsigned,              // UINT
                     boost::rational<int>,  // RATIONAL
                     float,                 // FLOAT
                     double,                // DOUBLE
                     std::string            // STRING
                     >
    variant_t;

class expressionEvaluator {
 private:
  /* data */
 public:
  expressionEvaluator(/* args */);
  ~expressionEvaluator();

  variant_t eval(std::list<token> program);
};
