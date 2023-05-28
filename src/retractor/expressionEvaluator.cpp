#include "expressionEvaluator.h"

#include <assert.h>
#include <rdb/payload.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>       // sqrt
#include <functional>  // std::function
#include <iostream>
#include <regex>
#include <stack>
#include <string>
#include <typeinfo>  // operator typeid
#include <variant>

#include "convertTypes.h"

cast<rdb::descFldVT> castFldVT;

expressionEvaluator::expressionEvaluator(/* args */) {}

expressionEvaluator::~expressionEvaluator() {}

typedef std::pair<rdb::descFldVT, rdb::descFldVT> pairVar;

pairVar normalize(const rdb::descFldVT& a, const rdb::descFldVT& b) {
  if (a.index() == b.index()) return pairVar(a, b);

  pairVar retVal;
  if (a.index() > b.index()) {
    return pairVar(a, castFldVT(b, static_cast<rdb::descFld>(a.index())));
  } else {
    return pairVar(castFldVT(a, static_cast<rdb::descFld>(b.index())), b);
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
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) {
                   retVal = std::make_pair(a.first + b.first, a.second + b.second);
                 },
                 [&retVal](std::pair<std::string, int> a, std::pair<std::string, int> b) {
                   retVal = std::make_pair(a.first + b.first, a.second + b.second);
                 },
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
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) {
                   retVal = std::make_pair(a.first - b.first, a.second - b.second);
                 },
                 [&retVal](std::pair<std::string, int> a, std::pair<std::string, int> b) {
                   retVal = std::make_pair(/* TODO? define str-str */ "?? -", a.second - b.second);
                 },
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
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) {
                   retVal = std::make_pair(a.first * b.first, a.second * b.second);
                 },
                 [&retVal](std::pair<std::string, int> a, std::pair<std::string, int> b) {
                   retVal = std::make_pair(/* TODO? define str*str */ "?? *", a.second * b.second);
                 },
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
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) {
                   retVal = std::make_pair(a.first / b.first, a.second / b.second);
                 },  //
                 [&retVal](std::pair<std::string, int> a, std::pair<std::string, int> b) {
                   retVal = std::make_pair(/* TODO? define str/str */ "?? /", a.second / b.second);
                 },  //
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
                 [&retVal](uint8_t a) { retVal = static_cast<uint8_t>(~a); },                                      // xor ?
                 [&retVal](int a) { retVal = -a; },                                                                //
                 [&retVal](unsigned a) { retVal = static_cast<unsigned>(~a); },                                    // xor ?
                 [&retVal](boost::rational<int> a) { retVal = boost::rational_cast<int>(-a); },                    //
                 [&retVal](float a) { retVal = -a; },                                                              //
                 [&retVal](double a) { retVal = -a; },                                                             //
                 [&retVal](std::vector<uint8_t> a) { SPDLOG_ERROR("TODO - vect8->T"); },                           //
                 [&retVal](std::vector<int> a) { SPDLOG_ERROR("TODO - vect-int->T"); },                            //
                 [&retVal](std::pair<int, int> a) { retVal = std::make_pair(-a.first, -a.second); },               //
                 [&retVal](std::pair<std::string, int> a) { retVal = std::make_pair("-" + a.first, -a.second); },  //
                 [&retVal](std::string a) { /* define neg of string ? */ }                                         //
             },
             inVar);

  return retVal;
}

rdb::descFldVT callFun(rdb::descFldVT& inVar, std::function<double(double)> fnName) {
  auto backResultType = inVar.index();
  if (backResultType >= rdb::BYTE && backResultType <= rdb::DOUBLE) {
    rdb::descFldVT floValue{fnName(std::get<double>(castFldVT(inVar, rdb::DOUBLE)))};
    return castFldVT(floValue, (rdb::descFld)backResultType);
  } else {
    // There is no definition of floor( std::string ) or sqrt( vector ) ?
    // throw error ?
    assert(false);
  }

  return inVar;
}

rdb::descFldVT expressionEvaluator::eval(std::list<token> program, rdb::payload* payload) {
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
        assert(false);
        break;
      case PUSH_ID2: { /* TODO */
        assert(payload != NULL);
        std::regex r("(\\w*)\\[(\\d*)\\]");
        std::smatch what;
        std::regex_search(tkStr, what, r);  // something[1]
        assert(what.size() == 3);
        // const std::string schema(what[1]);
        const std::string sOffset1(what[2]);
        const int offset1(atoi(sOffset1.c_str()));

        const auto anyValue = payload->getItem(offset1);
        // auto rdbtype = std::get<rdb::rtype>(payload->getDescriptor()[offset1]);

        rdb::descFldVT val = any_to_variant_cast(anyValue);
        rStack.push(val);
      } break;
      default:
        /* UNDEFINED */
        assert(false);
        break;
    }
  };

  return rStack.top();
}