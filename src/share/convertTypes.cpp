#include "convertTypes.h"

#include <spdlog/spdlog.h>

#include <istream>
#include <stack>
#include <string>
#include <type_traits>
#include <any>

template <typename T, typename K>
void visit_descFld(const K& inVar, K& retVal) {
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
template <typename T>
T cast<T>::operator()(const T& inVar, rdb::descFld reqType) {
  T retVal;
  switch (reqType) {
    case rdb::BAD:
      SPDLOG_ERROR("Unsupported bad cast");
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

boost::rational<int> Rationalize(const double inValue, const double DIFF /*=1E-6*/, const int ttl_const /*=11*/) {
  std::stack<int> st;
  double startx = inValue, diff, err1, err2;
  int ttl = ttl_const;
  unsigned int val;
  for (;;) {
    val = static_cast<unsigned int>(startx);
    st.push(static_cast<int>(val));
    if ((ttl--) == 0) break;
    diff = startx - val;
    if (diff < DIFF)
      break;
    else
      startx = 1 / diff;
    if (startx > (1 / DIFF)) break;
  }
  if (st.empty()) return boost::rational<int>(0, 1);
  boost::rational<int> result1(0, 1), result2(0, 1);
  while (!st.empty()) {
    if (result1.numerator() != 0)
      result2 = st.top() + (1 / result1);
    else
      result2 = st.top();
    st.pop();
    result1 = result2;
  }
  err1 = std::abs(rational_cast<double>(result1) - inValue);
  err2 = std::abs(rational_cast<double>(result2) - inValue);
  return err1 > err2 ? result2 : result1;
}

template struct cast<rdb::descFldVT>;
//template struct cast<std::any>;
