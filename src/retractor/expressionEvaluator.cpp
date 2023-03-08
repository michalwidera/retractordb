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
  // List of unsupported types by this function
  static_assert(!std::is_same_v<T, std::string>);
  static_assert(!std::is_same_v<T, boost::rational<int>>);
  static_assert(!std::is_same_v<T, std::vector<uint8_t>>);
  static_assert(!std::is_same_v<T, std::vector<int>>);

  std::visit(Overload{
                 [&retVal](std::monostate a) { retVal = 0; },                                 //
                 [&retVal](uint8_t a) { retVal = static_cast<T>(a); },                        //
                 [&retVal](int a) { retVal = static_cast<T>(a); },                            //
                 [&retVal](unsigned a) { retVal = static_cast<T>(a); },                       //
                 [&retVal](boost::rational<int> a) { retVal = boost::rational_cast<T>(a); },  //
                 [&retVal](float a) { retVal = static_cast<T>(a); },                          //
                 [&retVal](double a) { retVal = static_cast<T>(a); },                         //
                 [&retVal](std::vector<uint8_t> a) { SPDLOG_ERROR("TODO - vect8->T"); },      //
                 [&retVal](std::vector<int> a) { SPDLOG_ERROR("TODO - vect-int->T"); },       //
                 [&retVal](std::string a) { retVal = static_cast<T>(std::stoi(a)); }          //
             },
             inVar);
}

// https://stackoverflow.com/questions/23304177/c-alternative-for-parsing-input-with-sscanf
template <char C>
std::istream& expect(std::istream& in) {
  if ((in >> std::ws).peek() == C) {
    in.ignore();
  } else {
    in.setstate(std::ios_base::failbit);
  }
  return in;
}

// https://stackoverflow.com/questions/52088928/trying-to-return-the-value-from-stdvariant-using-stdvisit-and-a-lambda-expre
rdb::descFldVT cast(rdb::descFldVT inVar, rdb::descFld reqType) {
  rdb::descFldVT retVal;
  switch (reqType) {
    case rdb::BAD:
      SPDLOG_ERROR("Unspported bad cast");
      break;
    case rdb::BYTE:
      visit_descFld<uint8_t>(inVar, retVal);
      break;
    case rdb::INTEGER:
      visit_descFld<int>(inVar, retVal);
      break;
    case rdb::UINT:
      visit_descFld<unsigned>(inVar, retVal);
      break;
    case rdb::DOUBLE:
      visit_descFld<double>(inVar, retVal);
      break;
    case rdb::FLOAT:
      visit_descFld<float>(inVar, retVal);
      break;
    case rdb::RATIONAL:
      // Requested type is RATIONAL
      std::visit(Overload{
                     [&retVal](std::monostate a) { retVal = "NaN"; },                                //
                     [&retVal](uint8_t a) { retVal = boost::rational<int>(a); },                     //
                     [&retVal](int a) { retVal = boost::rational<int>(a); },                         //
                     [&retVal](unsigned a) { retVal = boost::rational<int>(static_cast<int>(a)); },  //
                     [&retVal](boost::rational<int> a) { retVal = a; },                              //
                     [&retVal](float a) { retVal = Rationalize(static_cast<double>(a)); },           //
                     [&retVal](double a) { retVal = Rationalize(a); },                               //
                     [&retVal](std::vector<uint8_t> a) { SPDLOG_ERROR("TODO - vect8->T"); },         //
                     [&retVal](std::vector<int> a) { SPDLOG_ERROR("TODO - vect-int->T"); },          //
                     [&retVal](std::string a) {
                       std::istringstream in(a);
                       int nom{0}, den{1};
                       in >> nom >> expect<'/'> >> den;
                       retVal = boost::rational<int>(nom, den);
                     }  //
                 },
                 inVar);
      break;
    case rdb::INTARRAY:
      // Requested type is std::vector<int>
      SPDLOG_ERROR("TODO - INTARRAY cast");
      std::visit(Overload{
                     [&retVal](std::monostate a) { retVal = "NaN"; },
                     [&retVal](uint8_t a) {
                       std::vector<int> r{static_cast<int>(a)};
                       retVal = r;
                     },
                     [&retVal](int a) {
                       std::vector<int> r{static_cast<int>(a)};
                       retVal = r;
                     },
                     [&retVal](unsigned a) {
                       std::vector<int> r{static_cast<int>(a)};
                       retVal = r;
                     },
                     [&retVal](boost::rational<int> a) {
                       std::vector<int> r{boost::rational_cast<int>(a)};
                       retVal = r;
                     },
                     [&retVal](float a) {
                       std::vector<int> r{static_cast<int>(a)};
                       retVal = r;
                     },
                     [&retVal](double a) {
                       std::vector<int> r{static_cast<int>(a)};
                       retVal = r;
                     },
                     [&retVal](std::vector<uint8_t> a) {
                       std::vector<int> r;
                       for (auto& v : a) r.push_back(static_cast<int>(v));
                       retVal = r;
                     },                                              //
                     [&retVal](std::vector<int> a) { retVal = a; },  //
                     [&retVal](std::string a) {}                     //
                 },
                 inVar);
      break;
    case rdb::BYTEARRAY:
      SPDLOG_ERROR("TODO - BYTEARRAY cast");
      break;
    case rdb::STRING:
      // Requested type is STRING
      std::visit(Overload{[&retVal](std::monostate a) { retVal = "NaN"; },                         //
                          [&retVal](uint8_t a) { retVal = std::to_string(a); },                    //
                          [&retVal](int a) { retVal = std::to_string(a); },                        //
                          [&retVal](unsigned a) { retVal = std::to_string(a); },                   //
                          [&retVal](float a) { retVal = std::to_string(a); },                      //
                          [&retVal](double a) { retVal = std::to_string(a); },                     //
                          [&retVal](std::vector<uint8_t> a) { SPDLOG_ERROR("TODO - vect8->T"); },  //
                          [&retVal](std::vector<int> a) { SPDLOG_ERROR("TODO - vect-int->T"); },   //
                          [&retVal](std::string a) { retVal = a; },                                //
                          [&retVal](boost::rational<int> a) {
                            std::stringstream ss;
                            ss << a;
                            retVal = ss.str();
                          }},
                 inVar);
      break;
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
        rStack.push(b - a);
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