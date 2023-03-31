#include "expressionEvaluator.h"

#include <assert.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>       // sqrt
#include <functional>  // std::function
#include <iostream>
#include <stack>
#include <string>
#include <typeinfo>  // operator typeid
#include <variant>

#include "convertTypes.h"

expressionEvaluator::expressionEvaluator(/* args */) {}

expressionEvaluator::~expressionEvaluator() {}

typedef std::pair<rdb::descFldVT, rdb::descFldVT> pairVar;

pairVar normalize(const rdb::descFldVT& a, const rdb::descFldVT& b) {
  if (a.index() == b.index()) return pairVar(a, b);

  pairVar retVal;
  if (a.index() > b.index()) {
    rdb::descFldVT b_conv;
    cast(b, b_conv, static_cast<rdb::descFld>(a.index()));
    return pairVar(a, b_conv);
  } else {
    rdb::descFldVT a_conv;
    cast(a, a_conv, static_cast<rdb::descFld>(b.index()));
    return pairVar(a_conv, b);
  }
}

rdb::descFldVT operator+(const rdb::descFldVT& aParam, const rdb::descFldVT& bParam) {
  rdb::descFldVT retVal{0};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](uint8_t a, uint8_t b) { retVal = a + b; },                            //
                 [&retVal](int a, int b) { retVal = a + b; },                                    //
                 [&retVal](unsigned a, unsigned b) { retVal = a + b; },                          //
                 [&retVal](std::string a, std::string b) { retVal = a + b; },                    //
                 [&retVal](double a, double b) { retVal = a + b; },                              //
                 [&retVal](float a, float b) { retVal = a + b; },                                //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) { retVal = a + b; },  //
                 [&retVal](std::vector<int> a, std::vector<int> b) {
                   std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::plus<int>());
                   retVal = a;
                 },
                 [&retVal](std::vector<uint8_t> a, std::vector<uint8_t> b) {
                   std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::plus<uint8_t>());
                   retVal = a;
                 },
                 [&retVal](auto a, auto b) { retVal = a + b; }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT operator-(const rdb::descFldVT& aParam, const rdb::descFldVT& bParam) {
  rdb::descFldVT retVal{0};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](uint8_t a, uint8_t b) { retVal = a - b; },                            //
                 [&retVal](int a, int b) { retVal = a - b; },                                    //
                 [&retVal](unsigned a, unsigned b) { retVal = a - b; },                          //
                 [&retVal](std::string a, std::string b) { /* define string-minus-string */ },   //
                 [&retVal](double a, double b) { retVal = a - b; },                              //
                 [&retVal](float a, float b) { retVal = a - b; },                                //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) { retVal = a - b; },  //
                 [&retVal](std::vector<int> a, std::vector<int> b) {
                   std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::minus<int>());
                   retVal = a;
                 },
                 [&retVal](std::vector<uint8_t> a, std::vector<uint8_t> b) {
                   std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::minus<uint8_t>());
                   retVal = a;
                 },
                 [&retVal](auto a, auto b) { retVal = a - b; }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT operator*(const rdb::descFldVT& aParam, const rdb::descFldVT& bParam) {
  rdb::descFldVT retVal{0};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](uint8_t a, uint8_t b) { retVal = a * b; },                            //
                 [&retVal](int a, int b) { retVal = a * b; },                                    //
                 [&retVal](unsigned a, unsigned b) { retVal = a * b; },                          //
                 [&retVal](std::string a, std::string b) { /* define string-mult-string */ },    //
                 [&retVal](double a, double b) { retVal = a * b; },                              //
                 [&retVal](float a, float b) { retVal = a * b; },                                //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) { retVal = a * b; },  //
                 [&retVal](std::vector<int> a, std::vector<int> b) {
                   std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::multiplies<int>());
                   retVal = a;
                 },
                 [&retVal](std::vector<uint8_t> a, std::vector<uint8_t> b) {
                   std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::multiplies<uint8_t>());
                   retVal = a;
                 },
                 [&retVal](auto a, auto b) { retVal = a * b; }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT operator/(const rdb::descFldVT& aParam, const rdb::descFldVT& bParam) {
  rdb::descFldVT retVal{0};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](uint8_t a, uint8_t b) { retVal = a / b; },                            //
                 [&retVal](int a, int b) { retVal = a / b; },                                    //
                 [&retVal](unsigned a, unsigned b) { retVal = a / b; },                          //
                 [&retVal](std::string a, std::string b) { /* define string-div-string */ },     //
                 [&retVal](double a, double b) { retVal = a / b; },                              //
                 [&retVal](float a, float b) { retVal = a / b; },                                //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) { retVal = a / b; },  //
                 [&retVal](std::vector<int> a, std::vector<int> b) {
                   std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::divides<int>());
                   retVal = a;
                 },
                 [&retVal](std::vector<uint8_t> a, std::vector<uint8_t> b) {
                   std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::divides<uint8_t>());
                   retVal = a;
                 },
                 [&retVal](auto a, auto b) { retVal = a / b; }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT neg(const rdb::descFldVT& inVar) {
  rdb::descFldVT retVal;

  std::visit(Overload{
                 [&retVal](std::monostate a) {},                                                 //
                 [&retVal](uint8_t a) { retVal = static_cast<uint8_t>(~a); },                    // xor ?
                 [&retVal](int a) { retVal = -a; },                                              //
                 [&retVal](unsigned a) { retVal = static_cast<unsigned>(~a); },                  // xor ?
                 [&retVal](boost::rational<int> a) { retVal = boost::rational_cast<int>(-a); },  //
                 [&retVal](float a) { retVal = -a; },                                            //
                 [&retVal](double a) { retVal = -a; },                                           //
                 [&retVal](std::vector<uint8_t> a) { SPDLOG_ERROR("TODO - vect8->T"); },         //
                 [&retVal](std::vector<int> a) { SPDLOG_ERROR("TODO - vect-int->T"); },          //
                 [&retVal](std::string a) { /* define neg of string ? */ }                       //
             },
             inVar);

  return retVal;
}

rdb::descFldVT callFun(rdb::descFldVT& inVar, std::function<double(double)> fnName) {
  auto backResultType = inVar.index();
  if (backResultType > rdb::BAD && backResultType <= rdb::DOUBLE) {
    rdb::descFldVT inVar_conv;
    cast(inVar, inVar_conv, rdb::DOUBLE);

    rdb::descFldVT floValue{fnName(std::get<double>(inVar_conv))};
    rdb::descFldVT floValue_conv;
    cast(floValue, floValue_conv, (rdb::descFld)backResultType);
    return floValue_conv;
  } else {
    // There is no definition of floor( std::string ) or sqrt( vector ) ?
    // throw error ?
    assert(false);
  }

  return inVar;
}

rdb::descFldVT expressionEvaluator::eval(std::list<token> program) {
  std::stack<rdb::descFldVT> rStack;
  rdb::descFldVT a, b;

  for (auto tk : program) {
    auto tkStr = tk.getStr_();
    switch (tk.getCommandID()) {
      case ADD:
      case SUBTRACT:
      case MULTIPLY:
      case DIVIDE:
        a = rStack.top();
        rStack.pop();
      case CALL:
      case NEGATE:
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
        rStack.push(b - a);
        break;
      case MULTIPLY:
        rStack.push(a * b);
        break;
      case DIVIDE:
        rStack.push(a / b);
        break;
      case NEGATE:
        rStack.push(neg(b));
        break;
      case CALL:
        // https://learnmoderncpp.com/2020/06/01/strings-as-switch-case-labels/ (?)
        if (tkStr == "floor")
          rStack.push(callFun(b, floor));
        else if (tkStr == "ceil")
          rStack.push(callFun(b, ceil));
        else if (tkStr == "sqrt")
          rStack.push(callFun(b, sqrt));
        else if (tkStr == "round")
          rStack.push(callFun(b, round));
        else if (tkStr == "sin")
          rStack.push(callFun(b, sin));
        else if (tkStr == "cos")
          rStack.push(callFun(b, cos));
        else if (tkStr == "tan")
          rStack.push(callFun(b, tan));
        else if (tkStr == "log")
          rStack.push(callFun(b, log));
        else if (tkStr == "log2")
          rStack.push(callFun(b, log2));
        else if (tkStr == "trunc")
          rStack.push(callFun(b, trunc));
        break;
      case PUSH_ID:
        /* TODO */
        break;
    }
  };

  return rStack.top();
}