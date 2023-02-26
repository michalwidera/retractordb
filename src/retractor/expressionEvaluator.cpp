#include "expressionEvaluator.h"

#include <assert.h>
#include <spdlog/spdlog.h>

expressionEvaluator::expressionEvaluator(/* args */) {}

expressionEvaluator::~expressionEvaluator() {}

rdb::variant_t neg(rdb::variant_t a) { return 0; };

rdb::variant_t operator+(const rdb::variant_t& a, const rdb::variant_t& b) {
  rdb::variant_t retVal;

  std::decay_t<decltype(a)> a_;
  std::decay_t<decltype(b)> b_;

  return retVal;
}

rdb::variant_t expressionEvaluator::eval(std::list<token> program) {
  rdb::variant_t retval;

  std::stack<rdb::variant_t> rStack;
  rdb::variant_t a, b;

  for (auto tk : program) {
    switch (tk.getCommandID()) {
      case ADD:
      case SUBTRACT:
      case MULTIPLY:
      case DIVIDE:
        a = rStack.top();
        rStack.pop();
        b = rStack.top();
        rStack.pop();
    }
    switch (tk.getCommandID()) {
      case PUSH_VAL:
        // rStack.push(tk.get());
        break;
      case ADD:
        rStack.push(a + b);
        break;
      case SUBTRACT:
        // rStack.push( b - a);
        break;
      case MULTIPLY:
        // rStack.push( a * b );
        break;
      case DIVIDE:
        // rStack.push( a / b );
        break;
      case NEGATE:
        a = rStack.top();
        rStack.pop();
        // rStack.push(neg(a));
        break;
    }
  };

  return retval;
}