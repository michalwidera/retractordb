#include "expressionEvaluator.h"

expressionEvaluator::expressionEvaluator(/* args */) {}

expressionEvaluator::~expressionEvaluator() {}

std::any add(std::any a, std::any b) { return 0; };
std::any sub(std::any a, std::any b) { return 0; };
std::any mul(std::any a, std::any b) { return 0; };
std::any div(std::any a, std::any b) { return 0; };
std::any neg(std::any a) { return 0; };

std::any expressionEvaluator::eval(std::list<token> program) {
  std::any retval;

  std::stack<std::any> rStack;
  std::any a, b;

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
        rStack.push(tk.get());
        break;
      case ADD:
        rStack.push(add(a, b));
        break;
      case SUBTRACT:
        rStack.push(sub(b, a));
        break;
      case MULTIPLY:
        rStack.push(mul(a, b));
        break;
      case DIVIDE:
        rStack.push(div(a, b));
        break;
      case NEGATE:
        a = rStack.top();
        rStack.pop();
        rStack.push(neg(a));
        break;
    }
  };

  return retval;
}