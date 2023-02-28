#include "expressionEvaluator.h"

#include <assert.h>
#include <spdlog/spdlog.h>

#include <iostream>

expressionEvaluator::expressionEvaluator(/* args */) {}

expressionEvaluator::~expressionEvaluator() {}

rdb::descFldVT neg(rdb::descFldVT a) { return 0; };

// helper type for the visitor #4
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

typedef std::pair<rdb::descFldVT, rdb::descFldVT> pairVar;

pairVar normalize(const rdb::descFldVT& a, const rdb::descFldVT& b) {
  pairVar retVal;
  if (a.index() > b.index()) {
    decltype(a) aRet = a;
    decltype(a) bRet;
    std::visit( //
    overloaded{ //
      [b](auto& arg) {
        bRet = b; }, //
      [b](const std::string& arg) {
        bRet = std::to_string(b); }, //
      [b](std::vector<auto>& arg) {
        bRet.push_back(b); } //
    ,a); //
      retVal = pairVar(aRet, bRet);
  } else {
      decltype(b) aRet;
      decltype(b) bRet = b;
      std::visit( //
      overloaded{ //
        [a](auto& arg) {
          aRet = a; }, //
        [a](const std::string& arg) {
          aRet = std::to_string(a); }, //
        [a](std::vector<auto>& arg) {
          aRet.push_back(a); } //
      ,b); //
        retVal = pairVar(aRet, bRet);
  }
  return retVal;
}

rdb::descFldVT operator+(const rdb::descFldVT& aParam, const rdb::descFldVT& bParam) {
      rdb::descFldVT retVal{0};

      auto [a, b] = normalize(aParam, bParam);

      const int* pval1 = std::get_if<int>(&a);
      const int* pval2 = std::get_if<int>(&b);

      const boost::rational<int>* pval1r = std::get_if<boost::rational<int>>(&a);
      const boost::rational<int>* pval2r = std::get_if<boost::rational<int>>(&b);

      if (pval1 && pval2) return *pval1 + *pval2;

      if (pval1r && pval2r) return *pval1r + *pval2r;

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