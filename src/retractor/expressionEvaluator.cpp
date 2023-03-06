#include "expressionEvaluator.h"

#include <assert.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <typeinfo>  // operator typeid
#include <variant>

expressionEvaluator::expressionEvaluator(/* args */) {}

expressionEvaluator::~expressionEvaluator() {}

rdb::descFldVT neg(rdb::descFldVT a) { return 0; };

typedef std::pair<rdb::descFldVT, rdb::descFldVT> pairVar;

// https://stackoverflow.com/questions/52088928/trying-to-return-the-value-from-stdvariant-using-stdvisit-and-a-lambda-expre
rdb::descFldVT cast(rdb::descFldVT inVar, rdb::descFld reqType) {
  rdb::descFldVT retVal;
  switch (reqType) {
    case rdb::BYTE: {
      uint8_t val;
      /* put code tak convert descFldVT into uint8_t */
      retVal = val;
    } break;
    case rdb::INTEGER: {
      int val;
      /* put code tak convert descFldVT into int */
      retVal = val;
    } break;
    case rdb::DOUBLE: {
      double val;
      /* put code tak convert descFldVT into double */
      retVal = val;
    } break;
    case rdb::STRING: {
      std::string val;
      /* put code tak convert descFldVT into string */
      retVal = val;
    } break;
    default:
      break;
  }
  return retVal;
}

pairVar normalize(const rdb::descFldVT& a, const rdb::descFldVT& b) {
  if (a.index() == b.index()) return pairVar(a, b);
  /*
    std::visit([&](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>)
            std::cout << arg;
        else if constexpr (std::is_same_v<T, std::string>)
            std::cout << std::quoted(arg);
    }, a);
  */

  pairVar retVal;
  if (a.index() > b.index()) {
    decltype(a) bRet = b;
    return pairVar(a, bRet);
  } else {
    decltype(b) aRet = a;
    return pairVar(aRet, b);
  }
}

rdb::descFldVT operator+(const rdb::descFldVT& aParam, const rdb::descFldVT& bParam) {
  rdb::descFldVT retVal{0};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](int a, int b) {
                   retVal = a + b;
                   std::cerr << "int-int" << std::endl;
                 },  //
                 [&retVal](std::string a, std::string b) {
                   std::cerr << "str-str" << std::endl;
                   retVal = a + b;
                 },  //
                 [&retVal](double a, double b) {
                   std::cerr << "dbl-dbl" << std::endl;
                   retVal = a + b;
                 },  //
                 [&retVal](std::vector<int> a, std::vector<int> b) {
                   std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::plus<int>());
                 },
                 [&retVal](std::vector<uint8_t> a, std::vector<uint8_t> b) {
                   std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::plus<uint8_t>());
                 },
                 [&retVal](auto a, auto b) {
                   retVal = a + b;
                   std::cerr << "auto-auto:" << typeid(a).name() << "-" << typeid(b).name() << std::endl;
                 }  //
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