#include "expressionEvaluator.h"

#include <assert.h>
#include <spdlog/spdlog.h>

#include <iostream>

expressionEvaluator::expressionEvaluator(/* args */) {}

expressionEvaluator::~expressionEvaluator() {}

rdb::variant_t neg(rdb::variant_t a) { return 0; };

typedef std::pair<rdb::variant_t, rdb::variant_t> pairVar_t;
pairVar_t normalize(const rdb::variant_t& a, const rdb::variant_t& b) {
  pairVar_t retVal;
  if (a.index() > b.index()) {
    decltype(a) aRet = a;
    decltype(a) bRet = b;
    retVal = pairVar_t(aRet, bRet);
  } else {
    decltype(b) aRet = a;
    decltype(b) bRet = b;
    retVal = pairVar_t(aRet, bRet);
  }
  return retVal;
}

rdb::variant_t operator+(const rdb::variant_t& aParam, const rdb::variant_t& bParam) {
  rdb::variant_t retVal{0};

  auto [a, b] = normalize(aParam, bParam);

  const int* pval1 = std::get_if<int>(&a);
  const int* pval2 = std::get_if<int>(&b);

  const boost::rational<int>* pval1r = std::get_if<boost::rational<int>>(&a);
  const boost::rational<int>* pval2r = std::get_if<boost::rational<int>>(&b);

  if (pval1 && pval2) return *pval1 + *pval2;

  if (pval1r && pval2r) return *pval1r + *pval2r;

  return retVal;
}

rdb::variant_t expressionEvaluator::eval(std::list<token> program) {
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
        rStack.push(tk.get());
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

  return rStack.top();
}