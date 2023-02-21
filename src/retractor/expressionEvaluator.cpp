#include "expressionEvaluator.h"

#include <spdlog/spdlog.h>

expressionEvaluator::expressionEvaluator(/* args */) {}

expressionEvaluator::~expressionEvaluator() {}

template <class V>
std::type_info const& var_type(V const& v) {
  return std::visit([](auto&& x) -> decltype(auto) { return typeid(x); }, v);
}

// This function will return two data that types are common and math-able
std::pair<variant_t, variant_t> normalize(variant_t a, variant_t b) {
  //SPDLOG_INFO("types {} {}", var_type(a), var_type(b));

  if (var_type(a) == var_type(b)) return std::pair<variant_t, variant_t>(a, b);

  return std::pair<variant_t, variant_t>(std::monostate{}, std::monostate{});
}

variant_t add(const variant_t &aParam, const variant_t &bParam) {
  auto [a, b]{normalize(aParam, aParam)};
  return 0;
};

variant_t sub(variant_t a, variant_t b) { return 0; };
variant_t mul(variant_t a, variant_t b) { return 0; };
variant_t div(variant_t a, variant_t b) { return 0; };
variant_t neg(variant_t a) { return 0; };

variant_t expressionEvaluator::eval(std::list<token> program) {
  variant_t retval;

  std::stack<variant_t> rStack;
  variant_t a, b;

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