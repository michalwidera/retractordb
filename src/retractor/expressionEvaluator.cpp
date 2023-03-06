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

template <typename T>
void visit_descFld(rdb::descFldVT& inVar, rdb::descFldVT& retVal) {
  std::visit(Overload{
                 [&retVal](std::monostate a) { retVal = 0; },                                 //
                 [&retVal](uint8_t a) { retVal = a; },                                        //
                 [&retVal](int a) { retVal = static_cast<T>(a); },                            //
                 [&retVal](unsigned a) { retVal = static_cast<T>(a); },                       //
                 [&retVal](boost::rational<int> a) { retVal = boost::rational_cast<T>(a); },  //
                 [&retVal](float a) { retVal = static_cast<T>(a); },                          //
                 [&retVal](double a) { retVal = static_cast<T>(a); },                         //
                 [&retVal](std::vector<uint8_t> a) { SPDLOG_ERROR("TODO - vect8->byte"); },   //
                 [&retVal](std::vector<int> a) { SPDLOG_ERROR("TODO - vect-int->byte"); },    //
                 [&retVal](std::string a) { retVal = static_cast<T>(std::stoi(a)); }          //
             },
             inVar);
}

// https://stackoverflow.com/questions/52088928/trying-to-return-the-value-from-stdvariant-using-stdvisit-and-a-lambda-expre
rdb::descFldVT cast(rdb::descFldVT inVar, rdb::descFld reqType) {
  rdb::descFldVT retVal;
  switch (reqType) {
    case rdb::BAD: {
      SPDLOG_ERROR("Unspported bad cast");
    } break;
    case rdb::BYTE: {
      visit_descFld<uint8_t>(inVar, retVal);
    } break;
    case rdb::INTEGER: {
      visit_descFld<int>(inVar, retVal);
    } break;
    case rdb::UINT: {
      visit_descFld<unsigned>(inVar, retVal);
    } break;
    case rdb::DOUBLE: {
      visit_descFld<double>(inVar, retVal);
    } break;
    case rdb::FLOAT: {
      visit_descFld<float>(inVar, retVal);
    } break;
    case rdb::RATIONAL: {
      SPDLOG_ERROR("TODO - rational cast");
    } break;
    case rdb::INTARRAY: {
      SPDLOG_ERROR("TODO - INTARRAY cast");
    } break;
    case rdb::BYTEARRAY: {
      SPDLOG_ERROR("TODO - BYTEARRAY cast");
    } break;
    case rdb::STRING: {
      std::visit(Overload{
                     [&retVal](std::monostate a) { retVal = "NaN"; },        //
                     [&retVal](uint8_t a) { retVal = std::to_string(a); },   //
                     [&retVal](int a) { retVal = std::to_string(a); },       //
                     [&retVal](unsigned a) { retVal = std::to_string(a); },  //
                     [&retVal](boost::rational<int> a) {
                       std::stringstream ss;
                       ss << a;
                       retVal = ss.str();
                     },                                                                          //
                     [&retVal](float a) { retVal = std::to_string(a); },                         //
                     [&retVal](double a) { retVal = std::to_string(a); },                        //
                     [&retVal](std::vector<uint8_t> a) { SPDLOG_ERROR("TODO - vect8->byte"); },  //
                     [&retVal](std::vector<int> a) { SPDLOG_ERROR("TODO - vect-int->byte"); },   //
                     [&retVal](std::string a) { retVal = a; }                                    //
                 },
                 inVar);
    } break;
    default:
      break;
  }
  return retVal;
}

pairVar normalize(const rdb::descFldVT& a, const rdb::descFldVT& b) {
  if (a.index() == b.index()) return pairVar(a, b);

  pairVar retVal;
  if (a.index() > b.index()) {
    return pairVar(a, cast(b, (rdb::descFld)a.index()));
  } else {
    return pairVar(cast(a, (rdb::descFld)b.index()), b);
  }
}

rdb::descFldVT operator+(const rdb::descFldVT& aParam, const rdb::descFldVT& bParam) {
  rdb::descFldVT retVal{0};

  auto [a, b] = normalize(aParam, bParam);

  assert(typeid(a) == typeid(b));

  std::visit(Overload{
                 [&retVal](int a, int b) {
                   retVal = a + b;
                   SPDLOG_INFO("[tx:{}-{}]", typeid(a).name(), typeid(b).name());
                 },  //
                 [&retVal](unit_8 a, unit_8 b) {
                   retVal = a + b;
                   SPDLOG_INFO("[tx:{}-{}]", typeid(a).name(), typeid(b).name());
                 },  //
                 [&retVal](unsigned a, unsigned b) {
                   retVal = a + b;
                   SPDLOG_INFO("[tx:{}-{}]", typeid(a).name(), typeid(b).name());
                 },  //
                 [&retVal](std::string a, std::string b) {
                   retVal = a + b;
                   SPDLOG_INFO("[tx:{}-{}]", typeid(a).name(), typeid(b).name());
                 },  //
                 [&retVal](double a, double b) {
                   retVal = a + b;
                   SPDLOG_INFO("[tx:{}-{}]", typeid(a).name(), typeid(b).name());
                 },  //
                 [&retVal](float a, float b) {
                   retVal = a + b;
                   SPDLOG_INFO("[tx:{}-{}]", typeid(a).name(), typeid(b).name());
                 },  //
                 [&retVal](std::vector<int> a, std::vector<int> b) {
                   std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::plus<int>());
                 },
                 [&retVal](std::vector<uint8_t> a, std::vector<uint8_t> b) {
                   std::transform(a.begin(), a.end(), b.begin(), a.begin(), std::plus<uint8_t>());
                 },
                 [&retVal](auto a, auto b) {
                   retVal = a + b;
                   SPDLOG_INFO("[tx:{}-{}]", typeid(a).name(), typeid(b).name());
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