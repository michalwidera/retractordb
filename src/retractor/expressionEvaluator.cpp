#include "expressionEvaluator.h"

#include <assert.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>
#include <typeinfo>  // operator typeid

expressionEvaluator::expressionEvaluator(/* args */) {}

expressionEvaluator::~expressionEvaluator() {}

rdb::descFldVT neg(rdb::descFldVT a) { return 0; };

typedef std::pair<rdb::descFldVT, rdb::descFldVT> pairVar;

pairVar normalize(const rdb::descFldVT& a, const rdb::descFldVT& b) {
  pairVar retVal;
  if (a.index() > b.index()) {
    decltype(a) aRet = a;
    decltype(a) bRet = b;
    retVal = pairVar(aRet, bRet);
  } else {
    decltype(b) aRet = a;
    decltype(b) bRet = b;
    retVal = pairVar(aRet, bRet);
  }
  static_assert(std::is_same<decltype(std::get<0>(retVal)), decltype(std::get<1>(retVal))>::value);

  return retVal;
}

rdb::descFldVT operator+(const rdb::descFldVT& aParam, const rdb::descFldVT& bParam) {
  rdb::descFldVT retVal{0};

  auto [a, b] = normalize(aParam, bParam);

  std::visit(Overload{
                 [&retVal](int a, int b) { retVal = a + b; },  //
                 [&retVal](std::string a, std::string b) { retVal = a + b; },
                 [&retVal](std::vector<int> a, std::vector<int> b) {
                   std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::plus<int>());
                 },
                 [&retVal](std::vector<uint8_t> a, std::vector<uint8_t> b) {
                   std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::plus<uint8_t>());
                 },
                 [&retVal](auto a, auto b) { retVal = a + b; }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT expressionEvaluator::eval(std::list<token> program) {
  std::stack<rdb::descFldVT> rStack;
  rdb::descFldVT a, b;

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
        rStack.push(tk.getVT());
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