#include "rdb/convertTypes.h"

#include <spdlog/spdlog.h>

#include <iostream>
#include <istream>
#include <optional>
#include <stack>
#include <string>
#include <type_traits>
#include <typeinfo>

template <typename T, typename K>
void visit_descFld(const K &inVar, K &retVal) {
  // List of unsupported types by this function
  static_assert(!std::is_same_v<T, boost::rational<int>>);
  static_assert(!std::is_same_v<T, std::pair<int, int>>);
  static_assert(!std::is_same_v<T, std::pair<std::string, int>>);

  if constexpr (std::is_same_v<K, rdb::descFldVT>) {
    std::visit(Overload{
                   [&retVal](uint8_t a) { retVal = static_cast<T>(a); },                                 //
                   [&retVal](int a) { retVal = static_cast<T>(a); },                                     //
                   [&retVal](unsigned a) { retVal = static_cast<T>(a); },                                //
                   [&retVal](boost::rational<int> a) { retVal = boost::rational_cast<T>(a); },           //
                   [&retVal](float a) { retVal = static_cast<T>(a); },                                   //
                   [&retVal](double a) { retVal = static_cast<T>(a); },                                  //
                   [&retVal](std::pair<int, int> a) { SPDLOG_ERROR("TODO - pair-int->T"); },             //
                   [&retVal](std::pair<std::string, int> a) { SPDLOG_ERROR("TODO - idxpair-int->T"); },  //
                   [&retVal](const std::string &a) {
                     try {
                       retVal = static_cast<T>(std::stoi(a));
                     } catch (std::exception &err) {
                       SPDLOG_ERROR("Cant conv nonint string to integer.");
                     };
                   }  //
               },
               inVar);
  } else {
    if (inVar.type() == typeid(uint8_t)) {
      retVal = static_cast<T>(std::any_cast<uint8_t>(inVar));
    } else if (inVar.type() == typeid(int)) {
      retVal = static_cast<T>(std::any_cast<int>(inVar));
    } else if (inVar.type() == typeid(unsigned)) {
      retVal = static_cast<T>(std::any_cast<unsigned>(inVar));
    } else if (inVar.type() == typeid(boost::rational<int>)) {
      retVal = boost::rational_cast<T>(std::any_cast<boost::rational<int>>(inVar));
    } else if (inVar.type() == typeid(float)) {
      retVal = static_cast<T>(std::any_cast<float>(inVar));
    } else if (inVar.type() == typeid(double)) {
      retVal = static_cast<T>(std::any_cast<double>(inVar));
    } else if (inVar.type() == typeid(std::pair<int, int>)) {
      SPDLOG_ERROR("No cast INTPAIR to any type here");
      retVal = static_cast<T>(0);
    } else if (inVar.type() == typeid(std::pair<std::string, int>)) {
      SPDLOG_ERROR("No cast IDXPAIR to any type here");
      retVal = static_cast<T>(0);
    } else if (inVar.type() == typeid(std::string)) {
      try {
        retVal = static_cast<T>(std::stoi(std::any_cast<std::string>(inVar)));
      } catch (std::exception &err) {
        SPDLOG_ERROR("Cant conv nonint string to integer here.");
      };
    } else {
      SPDLOG_ERROR("TODO - std::any->T");
    }
  }
}

// https://stackoverflow.com/questions/23304177/c-alternative-for-parsing-input-with-sscanf
template <char C>
std::istream &expect(std::istream &in) {
  if ((in >> std::ws).peek() == C) {
    in.ignore();
  } else {
    in.setstate(std::ios_base::failbit);
  }
  return in;
}

// https://stackoverflow.com/questions/52088928/trying-to-return-the-value-from-stdvariant-using-stdvisit-and-a-lambda-expre
template <typename T>
T cast<T>::operator()(const T &inVar, rdb::descFld reqType) {
  T retVal;
  switch (reqType) {
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
    case rdb::IDXPAIR:
      SPDLOG_ERROR("TODO - IDXPAIR->T");
      assert(false);
      break;
    case rdb::INTPAIR:
      // Requested type is INT PAIR
      if constexpr (std::is_same_v<T, rdb::descFldVT>) {
        std::visit(
            Overload{                                                                                                 //
                     [&retVal](uint8_t a) { retVal = std::make_pair(0, a); },                                         //
                     [&retVal](int a) { retVal = std::make_pair(0, a); },                                             //
                     [&retVal](unsigned a) { retVal = std::make_pair(0, static_cast<int>(a)); },                      //
                     [&retVal](boost::rational<int> a) { retVal = std::make_pair(a.numerator(), a.denominator()); },  //
                     [&retVal](float a) {
                       auto r = Rationalize(static_cast<double>(a));
                       retVal = std::make_pair(r.numerator(), r.denominator());
                     },
                     [&retVal](double a) {
                       auto r = Rationalize(a);
                       retVal = std::make_pair(r.numerator(), r.denominator());
                     },                                                                                                       //
                     [&retVal](std::pair<int, int> a) { retVal = a; },                                                        //
                     [&retVal](std::pair<std::string, int> a) { retVal = std::make_pair(atoi(a.first.c_str()), a.second); },  //
                     [&retVal](const std::string &a) {
                       std::istringstream in(a);
                       int first{0};
                       int second{1};
                       in >> first >> expect<','> >> second;
                       retVal = std::make_pair(first, second);
                     }},
            inVar);
      } else {
        if (inVar.type() == typeid(uint8_t)) {
          retVal = std::make_pair(0, std::any_cast<uint8_t>(inVar));
        } else if (inVar.type() == typeid(int)) {
          retVal = std::make_pair(0, std::any_cast<int>(inVar));
        } else if (inVar.type() == typeid(unsigned)) {
          retVal = std::make_pair(0, std::any_cast<unsigned>(inVar));
        } else if (inVar.type() == typeid(boost::rational<int>)) {
          auto r = std::any_cast<boost::rational<int>>(inVar);
          retVal = std::make_pair(r.numerator(), r.denominator());
        } else if (inVar.type() == typeid(float)) {
          auto r = Rationalize(std::any_cast<float>(inVar));
          retVal = std::make_pair(r.numerator(), r.denominator());
        } else if (inVar.type() == typeid(double)) {
          auto r = Rationalize(std::any_cast<double>(inVar));
          retVal = std::make_pair(r.numerator(), r.denominator());
        } else if (inVar.type() == typeid(std::pair<int, int>)) {
          retVal = std::any_cast<std::pair<int, int>>(inVar);
        } else if (inVar.type() == typeid(std::string)) {
          std::istringstream in(std::any_cast<std::string>(inVar));
          int first{0};
          int second{1};
          in >> first >> expect<','> >> second;
          retVal = std::make_pair(first, second);
        }
      }
      break;
    case rdb::RATIONAL:
      // Requested type is RATIONAL
      if constexpr (std::is_same_v<T, rdb::descFldVT>) {
        std::visit(Overload{                                                                                //
                            [&retVal](uint8_t a) { retVal = boost::rational<int>(a); },                     //
                            [&retVal](int a) { retVal = boost::rational<int>(a); },                         //
                            [&retVal](unsigned a) { retVal = boost::rational<int>(static_cast<int>(a)); },  //
                            [&retVal](boost::rational<int> a) { retVal = a; },                              //
                            [&retVal](float a) { retVal = Rationalize(static_cast<double>(a)); },           //
                            [&retVal](double a) { retVal = Rationalize(a); },                               //
                            [&retVal](std::pair<int, int> a) {
                              assert(a.second != 0);
                              retVal = boost::rational<int>(a.first, a.second);
                            },  //
                            [&retVal](std::pair<std::string, int> a) {
                              retVal = boost::rational<int>(a.second, 1);
                            },  //  first is skipped
                            [&retVal](const std::string &a) {
                              std::istringstream in(a);
                              int nom{0};
                              int den{1};
                              in >> nom >> expect<'/'> >> den;
                              assert(den != 0);
                              retVal = boost::rational<int>(nom, den);
                            }},
                   inVar);
      } else {
        if (inVar.type() == typeid(uint8_t)) {
          retVal = boost::rational<int>(std::any_cast<uint8_t>(inVar));
        } else if (inVar.type() == typeid(int)) {
          retVal = boost::rational<int>(std::any_cast<int>(inVar));
        } else if (inVar.type() == typeid(unsigned)) {
          retVal = boost::rational<int>(std::any_cast<unsigned>(inVar));
        } else if (inVar.type() == typeid(boost::rational<int>)) {
          retVal = std::any_cast<boost::rational<int>>(inVar);
        } else if (inVar.type() == typeid(float)) {
          retVal = Rationalize(std::any_cast<float>(inVar));
        } else if (inVar.type() == typeid(double)) {
          retVal = Rationalize(std::any_cast<double>(inVar));
        } else if (inVar.type() == typeid(std::pair<int, int>)) {
          std::pair pairVar = std::any_cast<std::pair<int, int>>(inVar);
          assert(pairVar.second != 0);
          retVal = boost::rational<int>(pairVar.first, pairVar.second);
        } else if (inVar.type() == typeid(std::string)) {
          std::istringstream in(std::any_cast<std::string>(inVar));
          int nom{0};
          int den{1};
          in >> nom >> expect<'/'> >> den;
          assert(den != 0);
          retVal = boost::rational<int>(nom, den);
        }
      }
      break;
    case rdb::STRING:
      // Requested type is STRING
      if constexpr (std::is_same_v<T, rdb::descFldVT>) {
        std::visit(
            Overload{                                                                                                          //
                     [&retVal](uint8_t a) { retVal = std::to_string(a); },                                                     //
                     [&retVal](int a) { retVal = std::to_string(a); },                                                         //
                     [&retVal](unsigned a) { retVal = std::to_string(a); },                                                    //
                     [&retVal](float a) { retVal = std::to_string(a); },                                                       //
                     [&retVal](double a) { retVal = std::to_string(a); },                                                      //
                     [&retVal](std::pair<int, int> a) { retVal = std::to_string(a.first) + "," + std::to_string(a.second); },  //
                     [&retVal](std::pair<std::string, int> a) { retVal = a.first + "," + std::to_string(a.second); },          //
                     [&retVal](const std::string &a) { retVal = a; },                                                          //
                     [&retVal](boost::rational<int> a) {
                       std::stringstream ss;
                       ss << a;
                       retVal = ss.str();
                     }},
            inVar);
      } else {
        if (inVar.type() == typeid(uint8_t)) {
          retVal = std::to_string(std::any_cast<uint8_t>(inVar));
        } else if (inVar.type() == typeid(int)) {
          retVal = std::to_string(std::any_cast<int>(inVar));
        } else if (inVar.type() == typeid(unsigned)) {
          retVal = std::to_string(std::any_cast<unsigned>(inVar));
        } else if (inVar.type() == typeid(boost::rational<int>)) {
          std::stringstream ss;
          ss << std::any_cast<boost::rational<int>>(inVar);
          retVal = ss.str();
        } else if (inVar.type() == typeid(float)) {
          retVal = std::to_string(std::any_cast<float>(inVar));
        } else if (inVar.type() == typeid(double)) {
          retVal = std::to_string(std::any_cast<double>(inVar));
        } else if (inVar.type() == typeid(std::string)) {
          retVal = inVar;
        } else if (inVar.type() == typeid(std::pair<int, int>)) {
          std::pair pairVar = std::any_cast<std::pair<int, int>>(inVar);
          retVal            = std::to_string(pairVar.first) + "," + std::to_string(pairVar.second);
        } else if (inVar.type() == typeid(std::pair<std::string, int>)) {
          std::pair pairVar = std::any_cast<std::pair<std::string, int>>(inVar);
          retVal            = pairVar.first + "," + std::to_string(pairVar.second);
        } else {
          SPDLOG_ERROR("TODO - std::any->T");
        }
      }
      break;

    default:
      break;
  }
  return retVal;
}

boost::rational<int> Rationalize(const double inValue, const double DIFF /*=1E-6*/, const int ttl_const /*=11*/) {
  std::stack<int> st;
  double startx = inValue, diff, err1, err2;
  int ttl       = ttl_const;
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

// based: https://stackoverflow.com/questions/61182946/convert-stdany-to-stdvariant
rdb::descFldVT any_to_variant_cast(std::any a) {
  if (!a.has_value()) throw std::bad_any_cast();

  if (a.type() == typeid(std::string)) return std::any_cast<std::string>(a);
  if (a.type() == typeid(int)) return std::any_cast<int>(a);
  if (a.type() == typeid(uint8_t)) return std::any_cast<uint8_t>(a);
  if (a.type() == typeid(unsigned)) return std::any_cast<unsigned>(a);
  if (a.type() == typeid(double)) return std::any_cast<double>(a);
  if (a.type() == typeid(float)) return std::any_cast<float>(a);
  if (a.type() == typeid(boost::rational<int>)) return std::any_cast<boost::rational<int>>(a);
  throw std::bad_any_cast();
  return rdb::descFldVT(0);  // Proforma
}

template struct cast<rdb::descFldVT>;
template struct cast<std::any>;
