#include "expressionEvaluator.h"

#include <spdlog/spdlog.h>

#include <cassert>
#include <cmath>       // sqrt
#include <functional>  // std::function
#include <optional>
#include <regex>
#include <stack>
#include <stdexcept>
#include <string>
#include <typeinfo>  // operator typeid
#include <variant>

#include "rdb/convertTypes.h"

static cast<rdb::descFldVT> castFldVT;

expressionEvaluator::expressionEvaluator(/* args */) {}

expressionEvaluator::~expressionEvaluator() {}

typedef std::pair<rdb::descFldVT, rdb::descFldVT> pairVar;

bool isNullValue(const rdb::descFldVT &value) { return std::holds_alternative<std::monostate>(value); }

std::optional<bool> toLogicValue(const rdb::descFldVT &value) {
  return std::visit(
      Overload{[](std::monostate) -> std::optional<bool> { return std::nullopt; },
               [](uint8_t a) -> std::optional<bool> { return a != 0; }, [](int a) -> std::optional<bool> { return a != 0; },
               [](unsigned a) -> std::optional<bool> { return a != 0; },
               [](double a) -> std::optional<bool> { return a != 0.0; },
               [](float a) -> std::optional<bool> { return a != 0.0f; },
               [](boost::rational<int> a) -> std::optional<bool> { return a != boost::rational<int>(0); },
               [](auto) -> std::optional<bool> {
                 assert(false && "Unsupported type for logic evaluation");
                 return false;
               }},
      value);
}

rdb::descFldVT logicResultAsType(bool value, const rdb::descFldVT &typeRef) {
  return std::visit(Overload{[value](uint8_t) -> rdb::descFldVT { return static_cast<uint8_t>(value ? 1 : 0); },
                             [value](int) -> rdb::descFldVT { return value ? 1 : 0; },
                             [value](unsigned) -> rdb::descFldVT { return value ? 1U : 0U; },
                             [value](double) -> rdb::descFldVT { return value ? 1.0 : 0.0; },
                             [value](float) -> rdb::descFldVT { return value ? 1.0f : 0.0f; },
                             [value](boost::rational<int>) -> rdb::descFldVT { return boost::rational<int>(value ? 1 : 0); },
                             [](std::monostate) -> rdb::descFldVT { return std::monostate{}; },
                             [](auto) -> rdb::descFldVT {
                               assert(false && "Unsupported type for logic result conversion");
                               return std::monostate{};
                             }},
                    typeRef);
}

const rdb::descFldVT &logicResultTypeRef(const rdb::descFldVT &a, const rdb::descFldVT &b) {
  if (!isNullValue(a)) return a;
  if (!isNullValue(b)) return b;
  return a;
}

pairVar normalize(const rdb::descFldVT &a, const rdb::descFldVT &b) {
  if (a.index() == b.index()) return pairVar(a, b);

  pairVar retVal;
  if (a.index() > b.index()) {
    return pairVar(a, castFldVT(b, static_cast<rdb::descFld>(a.index())));
  } else {
    return pairVar(castFldVT(a, static_cast<rdb::descFld>(b.index())), b);
  }
}

rdb::descFldVT operator+(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },       //
                 [&retVal](uint8_t a, uint8_t b) { retVal = a + b; },                            //
                 [&retVal](int a, int b) { retVal = a + b; },                                    //
                 [&retVal](unsigned a, unsigned b) { retVal = a + b; },                          //
                 [&retVal](const std::string &a, const std::string &b) { retVal = a + b; },      //
                 [&retVal](double a, double b) { retVal = a + b; },                              //
                 [&retVal](float a, float b) { retVal = a + b; },                                //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) { retVal = a + b; },  //
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) {
                   retVal = std::make_pair(a.first + b.first, a.second + b.second);
                 },
                 [&retVal](std::pair<std::string, int> a, std::pair<std::string, int> b) {
                   retVal = std::make_pair(a.first + b.first, a.second + b.second);
                 },
                 [&retVal](auto a, auto b) { retVal = a + b; }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT operator-(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                    //
                 [&retVal](uint8_t a, uint8_t b) { retVal = a - b; },                                         //
                 [&retVal](int a, int b) { retVal = a - b; },                                                 //
                 [&retVal](unsigned a, unsigned b) { retVal = a - b; },                                       //
                 [&retVal](const std::string &a, const std::string &b) { /* define string-minus-string */ },  //
                 [&retVal](double a, double b) { retVal = a - b; },                                           //
                 [&retVal](float a, float b) { retVal = a - b; },                                             //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) { retVal = a - b; },               //
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) {
                   retVal = std::make_pair(a.first - b.first, a.second - b.second);
                 },
                 [&retVal](std::pair<std::string, int> a, std::pair<std::string, int> b) {
                   retVal = std::make_pair(/* TODO? define str-str */ "?? -", a.second - b.second);
                 },
                 [&retVal](auto a, auto b) { retVal = a - b; }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT operator*(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                   //
                 [&retVal](uint8_t a, uint8_t b) { retVal = a * b; },                                        //
                 [&retVal](int a, int b) { retVal = a * b; },                                                //
                 [&retVal](unsigned a, unsigned b) { retVal = a * b; },                                      //
                 [&retVal](const std::string &a, const std::string &b) { /* define string-mult-string */ },  //
                 [&retVal](double a, double b) { retVal = a * b; },                                          //
                 [&retVal](float a, float b) { retVal = a * b; },                                            //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) { retVal = a * b; },              //
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) {
                   retVal = std::make_pair(a.first * b.first, a.second * b.second);
                 },
                 [&retVal](std::pair<std::string, int> a, std::pair<std::string, int> b) {
                   retVal = std::make_pair(/* TODO? define str*str */ "?? *", a.second * b.second);
                 },
                 [&retVal](auto a, auto b) { retVal = a * b; }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT operator/(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  const bool divisorIsZero =
      std::visit(Overload{[](uint8_t v) { return v == 0; }, [](int v) { return v == 0; }, [](unsigned v) { return v == 0U; },
                          [](double v) { return v == 0.0; }, [](float v) { return v == 0.0f; },
                          [](boost::rational<int> v) { return v == boost::rational<int>(0); },
                          [](std::monostate) { return false; }, [](auto) { return false; }},
                 b);

  if (divisorIsZero) {
    throw std::domain_error("Division by zero in expressionEvaluator");
  }

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                  //
                 [&retVal](uint8_t a, uint8_t b) { retVal = a / b; },                                       //
                 [&retVal](int a, int b) { retVal = a / b; },                                               //
                 [&retVal](unsigned a, unsigned b) { retVal = a / b; },                                     //
                 [&retVal](const std::string &a, const std::string &b) { /* define string-div-string */ },  //
                 [&retVal](double a, double b) { retVal = a / b; },                                         //
                 [&retVal](float a, float b) { retVal = a / b; },                                           //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) { retVal = a / b; },             //
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) {
                   retVal = std::make_pair(a.first / b.first, a.second / b.second);
                 },  //
                 [&retVal](std::pair<std::string, int> a, std::pair<std::string, int> b) {
                   retVal = std::make_pair(/* TODO? define str/str */ "?? /", a.second / b.second);
                 },                                             //
                 [&retVal](auto a, auto b) { retVal = a / b; }  //
             },
             a, b);

  return retVal;
}

rdb::descFldVT is_eq(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                  //
                 [&retVal](uint8_t a, uint8_t b) { retVal = (a == b) ? uint8_t(1) : uint8_t(0); },          //
                 [&retVal](int a, int b) { retVal = (a == b) ? int(1) : int(0); },                          //
                 [&retVal](unsigned a, unsigned b) { retVal = (a == b) ? unsigned(1) : unsigned(0); },      //
                 [&retVal](const std::string &a, const std::string &b) { retVal = (a == b) ? "1" : "0"; },  //
                 [&retVal](double a, double b) { retVal = (a == b) ? double(1) : double(0); },              //
                 [&retVal](float a, float b) { retVal = (a == b) ? float(1) : float(0); },                  //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) {
                   retVal = (a == b) ? boost::rational<int>(1) : boost::rational<int>(0);
                 },                                                                                                            //
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) { assert(false && "no support."); },                  //
                 [&retVal](std::pair<std::string, int> a, std::pair<std::string, int> b) { assert(false && "no support."); },  //
                 [&retVal](auto a, auto b) { assert(false && "no support."); }                                                 //
             },
             a, b);

  return retVal;
}

rdb::descFldVT is_neq(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                  //
                 [&retVal](uint8_t a, uint8_t b) { retVal = (a != b) ? uint8_t(1) : uint8_t(0); },          //
                 [&retVal](int a, int b) { retVal = (a != b) ? int(1) : int(0); },                          //
                 [&retVal](unsigned a, unsigned b) { retVal = (a != b) ? unsigned(1) : unsigned(0); },      //
                 [&retVal](const std::string &a, const std::string &b) { retVal = (a != b) ? "1" : "0"; },  //
                 [&retVal](double a, double b) { retVal = (a != b) ? double(1) : double(0); },              //
                 [&retVal](float a, float b) { retVal = (a != b) ? float(1) : float(0); },                  //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) {
                   retVal = (a != b) ? boost::rational<int>(1) : boost::rational<int>(0);
                 },                                                                                                            //
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) { assert(false && "no support."); },                  //
                 [&retVal](std::pair<std::string, int> a, std::pair<std::string, int> b) { assert(false && "no support."); },  //
                 [&retVal](auto a, auto b) { assert(false && "no support."); }                                                 //
             },
             a, b);

  return retVal;
}

rdb::descFldVT is_lt(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                 //
                 [&retVal](uint8_t a, uint8_t b) { retVal = (a < b) ? uint8_t(1) : uint8_t(0); },          //
                 [&retVal](int a, int b) { retVal = (a < b) ? int(1) : int(0); },                          //
                 [&retVal](unsigned a, unsigned b) { retVal = (a < b) ? unsigned(1) : unsigned(0); },      //
                 [&retVal](const std::string &a, const std::string &b) { retVal = (a < b) ? "1" : "0"; },  //
                 [&retVal](double a, double b) { retVal = (a < b) ? double(1) : double(0); },              //
                 [&retVal](float a, float b) { retVal = (a < b) ? float(1) : float(0); },                  //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) {
                   retVal = (a < b) ? boost::rational<int>(1) : boost::rational<int>(0);
                 },                                                                                                            //
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) { assert(false && "no support."); },                  //
                 [&retVal](std::pair<std::string, int> a, std::pair<std::string, int> b) { assert(false && "no support."); },  //
                 [&retVal](auto a, auto b) { assert(false && "no support."); }                                                 //
             },
             a, b);

  return retVal;
}

rdb::descFldVT is_gt(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                 //
                 [&retVal](uint8_t a, uint8_t b) { retVal = (a > b) ? uint8_t(1) : uint8_t(0); },          //
                 [&retVal](int a, int b) { retVal = (a > b) ? int(1) : int(0); },                          //
                 [&retVal](unsigned a, unsigned b) { retVal = (a > b) ? unsigned(1) : unsigned(0); },      //
                 [&retVal](const std::string &a, const std::string &b) { retVal = (a > b) ? "1" : "0"; },  //
                 [&retVal](double a, double b) { retVal = (a > b) ? double(1) : double(0); },              //
                 [&retVal](float a, float b) { retVal = (a > b) ? float(1) : float(0); },                  //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) {
                   retVal = (a > b) ? boost::rational<int>(1) : boost::rational<int>(0);
                 },                                                                                                            //
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) { assert(false && "no support."); },                  //
                 [&retVal](std::pair<std::string, int> a, std::pair<std::string, int> b) { assert(false && "no support."); },  //
                 [&retVal](auto a, auto b) { assert(false && "no support."); }                                                 //
             },
             a, b);

  return retVal;
}

rdb::descFldVT is_le(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                  //
                 [&retVal](uint8_t a, uint8_t b) { retVal = (a >= b) ? uint8_t(1) : uint8_t(0); },          //
                 [&retVal](int a, int b) { retVal = (a >= b) ? int(1) : int(0); },                          //
                 [&retVal](unsigned a, unsigned b) { retVal = (a >= b) ? unsigned(1) : unsigned(0); },      //
                 [&retVal](const std::string &a, const std::string &b) { retVal = (a >= b) ? "1" : "0"; },  //
                 [&retVal](double a, double b) { retVal = (a >= b) ? double(1) : double(0); },              //
                 [&retVal](float a, float b) { retVal = (a >= b) ? float(1) : float(0); },                  //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) {
                   retVal = (a >= b) ? boost::rational<int>(1) : boost::rational<int>(0);
                 },                                                                                                            //
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) { assert(false && "no support."); },                  //
                 [&retVal](std::pair<std::string, int> a, std::pair<std::string, int> b) { assert(false && "no support."); },  //
                 [&retVal](auto a, auto b) { assert(false && "no support."); }                                                 //
             },
             a, b);

  return retVal;
}

rdb::descFldVT is_ge(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  rdb::descFldVT retVal{0};
  if (isNullValue(aParam) || isNullValue(bParam)) return std::monostate{};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](std::monostate, std::monostate) { retVal = std::monostate{}; },                  //
                 [&retVal](uint8_t a, uint8_t b) { retVal = (a <= b) ? uint8_t(1) : uint8_t(0); },          //
                 [&retVal](int a, int b) { retVal = (a <= b) ? int(1) : int(0); },                          //
                 [&retVal](unsigned a, unsigned b) { retVal = (a <= b) ? unsigned(1) : unsigned(0); },      //
                 [&retVal](const std::string &a, const std::string &b) { retVal = (a <= b) ? "1" : "0"; },  //
                 [&retVal](double a, double b) { retVal = (a <= b) ? double(1) : double(0); },              //
                 [&retVal](float a, float b) { retVal = (a <= b) ? float(1) : float(0); },                  //
                 [&retVal](boost::rational<int> a, boost::rational<int> b) {
                   retVal = (a <= b) ? boost::rational<int>(1) : boost::rational<int>(0);
                 },                                                                                                            //
                 [&retVal](std::pair<int, int> a, std::pair<int, int> b) { assert(false && "no support."); },                  //
                 [&retVal](std::pair<std::string, int> a, std::pair<std::string, int> b) { assert(false && "no support."); },  //
                 [&retVal](auto a, auto b) { assert(false && "no support."); }                                                 //
             },
             a, b);

  return retVal;
}

rdb::descFldVT is_logic_or(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  auto a = toLogicValue(aParam);
  auto b = toLogicValue(bParam);

  if ((a.has_value() && *a) || (b.has_value() && *b)) {
    return logicResultAsType(true, logicResultTypeRef(aParam, bParam));
  }

  if (a.has_value() && b.has_value()) {
    return logicResultAsType(false, logicResultTypeRef(aParam, bParam));
  }

  return std::monostate{};
}

rdb::descFldVT is_logic_and(const rdb::descFldVT &aParam, const rdb::descFldVT &bParam) {
  auto a = toLogicValue(aParam);
  auto b = toLogicValue(bParam);

  if ((a.has_value() && !*a) || (b.has_value() && !*b)) {
    return logicResultAsType(false, logicResultTypeRef(aParam, bParam));
  }

  if (a.has_value() && b.has_value()) {
    return logicResultAsType(true, logicResultTypeRef(aParam, bParam));
  }

  return std::monostate{};
}

rdb::descFldVT neg(const rdb::descFldVT &inVar) {
  rdb::descFldVT retVal;
  if (isNullValue(inVar)) return std::monostate{};

  std::visit(Overload{
                 [&retVal](std::monostate) { retVal = std::monostate{}; },                                         //
                 [&retVal](uint8_t a) { retVal = static_cast<uint8_t>(~a); },                                      // xor ?
                 [&retVal](int a) { retVal = -a; },                                                                //
                 [&retVal](unsigned a) { retVal = static_cast<unsigned>(~a); },                                    // xor ?
                 [&retVal](boost::rational<int> a) { retVal = boost::rational_cast<int>(-a); },                    //
                 [&retVal](float a) { retVal = -a; },                                                              //
                 [&retVal](double a) { retVal = -a; },                                                             //
                 [&retVal](std::pair<int, int> a) { retVal = std::make_pair(-a.first, -a.second); },               //
                 [&retVal](std::pair<std::string, int> a) { retVal = std::make_pair("-" + a.first, -a.second); },  //
                 [&retVal](const std::string &a) { assert(false && "no support."); }                               //
             },
             inVar);

  return retVal;
}

rdb::descFldVT logic_not(const rdb::descFldVT &inVar) {
  auto value = toLogicValue(inVar);
  if (!value.has_value()) return std::monostate{};
  return logicResultAsType(!(*value), inVar);
}

rdb::descFldVT callFun(rdb::descFldVT &inVar, std::function<double(double)> fnName) {
  if (isNullValue(inVar)) return std::monostate{};
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

rdb::descFldVT expressionEvaluator::eval(std::list<token> program, rdb::payload *payload) {
  std::stack<rdb::descFldVT> rStack;
  rdb::descFldVT a, b;

  auto popOrThrow = [&rStack](const char *opName) -> rdb::descFldVT {
    if (rStack.empty()) {
      throw std::runtime_error(std::string("Invalid expression: missing operand for ") + opName);
    }
    auto v = rStack.top();
    rStack.pop();
    return v;
  };

  for (auto tk : program) {
    auto tkStr = tk.getStr_();
    switch (tk.getCommandID()) {
      case ADD:
      case SUBTRACT:
      case MULTIPLY:
      case DIVIDE:
      case CMP_EQUAL:
      case CMP_NOT_EQUAL:
      case CMP_LT:
      case CMP_GT:
      case CMP_LE:
      case CMP_GE:
      case OR:
      case AND:
        a = popOrThrow("binary operator");
      case CALL:
      case NEGATE:
      case NOT:
        b = popOrThrow("unary operator");
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
        rStack.push(b / a);
        break;
      case NEGATE:
        rStack.push(neg(b));
        break;
      case NOT:
        rStack.push(logic_not(b));
        break;
      case CMP_EQUAL: {
        rStack.push(is_eq(b, a));
      }; break;
      case CMP_NOT_EQUAL:
        rStack.push(is_neq(b, a));
        break;
      case CMP_LT:
        rStack.push(is_lt(b, a));
        break;
      case CMP_GT:
        rStack.push(is_gt(b, a));
        break;
      case CMP_LE:
        rStack.push(is_le(b, a));
        break;
      case CMP_GE:
        rStack.push(is_ge(b, a));
        break;
      case OR:
        rStack.push(is_logic_or(b, a));
        break;
      case AND:
        rStack.push(is_logic_and(b, a));
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
        else
          throw std::runtime_error(std::string("Unsupported function call: ") + tkStr);
        break;
      case PUSH_ID: {
        assert(payload != nullptr);
        auto instancePosition = get<std::pair<std::string, int>>(tk.getVT());
        auto anyValueOpt      = payload->getItem(instancePosition.second);
        if (!anyValueOpt.has_value()) {
          rStack.push(std::monostate{});
          break;
        }
        rdb::descFldVT val = any_to_variant_cast(anyValueOpt.value());
        rStack.push(val);
      } break;
      case PUSH_IDX:
        SPDLOG_ERROR("There should not appear PUSH_IDX here.");
        assert(false && "::eval - PUSH_IDX should be translated to other PUSH_");
        abort();
        break;
      case PUSH_ID2: {
        assert(payload != nullptr);
        std::regex r("(\\w*)\\[(\\d*)\\]");
        std::smatch what;
        std::regex_search(tkStr, what, r);  // something[1]
        assert(what.size() == 3);
        // const std::string schema(what[1]);
        const std::string sOffset1(what[2]);
        const int offset1(atoi(sOffset1.c_str()));

        auto anyValueOpt = payload->getItem(offset1);
        if (!anyValueOpt.has_value()) {
          rStack.push(std::monostate{});
          break;
        }

        rdb::descFldVT val = any_to_variant_cast(anyValueOpt.value());
        rStack.push(val);
      } break;
      default:
        throw std::runtime_error("Unsupported token in expressionEvaluator");
    }
  };

  if (rStack.empty()) {
    throw std::runtime_error("Invalid expression: empty evaluation stack");
  }
  if (rStack.size() != 1) {
    throw std::runtime_error("Invalid expression: too many values on evaluation stack");
  }

  return rStack.top();
}  // end fn
