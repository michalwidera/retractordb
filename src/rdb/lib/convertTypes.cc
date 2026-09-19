#include "rdb/convertTypes.hpp"

#include <spdlog/spdlog.h>

#include "fatalError.hpp"

#include <charconv>
#include <cmath>
#include <istream>
#include <limits>
#include <stack>
#include <string>
#include <type_traits>
#include <typeinfo>

/// Zwezenie ZMIENNOPRZECINKOWE -> CALKOWITE bez zachowania nieokreslonego.
///
/// `static_cast<int>(1e30)` jest w C++ zachowaniem NIEOKRESLONYM, a procesory rozstrzygaja
/// je ROZNIE: x86-64 (cvttsd2si) oddaje wzorzec "integer indefinite", czyli INT_MIN, a
/// arm64 (fcvtzs) NASYCA do INT_MAX; dla NaN jest to odpowiednio INT_MIN i 0. Ta sama baza
/// i to samo zapytanie dawaly wiec na dwoch maszynach rozne liczby, po cichu i bez bledu.
/// Apple wymienia to wprost jako roznice do audytu przy przenosinach na arm64
/// ("Audit Code that Contains Float-to-Int Conversions", Addressing architectural
/// differences in your macOS code), a Arm jako jeden z dwoch dozwolonych rozjazdow miedzy
/// architekturami (Floating-point behavior learning path).
///
/// Regula: NASYCENIE do granic typu, NaN -> 0. To zachowanie arm64 zapisane WPROST, wiec
/// macOS nie zmienia wyniku, a Linux przestaje oddawac liczbe ujemna za ogromna dodatnia.
/// Gdyby wynik spoza zakresu mial byc NULL (regula checkedArith dla przepelnienia
/// arytmetyki), miejscem na to jest `to_integer` w ewaluatorze, gdzie NULL jest wyrazalny -
/// nie ten konwerter, ktory oddaje wartosc takze do std::any i do slotow rekordu.
///
/// Granice licza sie PO rzucie na typ zrodlowy, bo `max()` nie zawsze da sie w nim zapisac:
/// `static_cast<float>(INT_MAX)` to 2^31, czyli juz poza zakresem int. Porownanie
/// nieostre wzgledem tak policzonej granicy jest wiec poprawne w obie strony.
template <typename T, typename F>
static T narrowFloatTo(F value) {
  static_assert(std::is_floating_point_v<F>);

  if constexpr (std::is_floating_point_v<T>) {
    return static_cast<T>(value);
  } else {
    if (std::isnan(value)) return T{0};
    if (value >= static_cast<F>(std::numeric_limits<T>::max())) return std::numeric_limits<T>::max();
    if (value <= static_cast<F>(std::numeric_limits<T>::lowest())) return std::numeric_limits<T>::lowest();
    return static_cast<T>(value);
  }
}

template <typename T, typename K>
static void parse_string(const std::string &a, K &retVal) {
  using P = std::conditional_t<std::is_floating_point_v<T>, double, int>;
  P val{};
  if (auto [p, e] = std::from_chars(a.data(), a.data() + a.size(), val); e == std::errc{})
    retVal = static_cast<T>(val);
  else {
    SPDLOG_ERROR("Cant conv string to numeric type.");
    retVal = std::monostate{};
  }
}

template <typename T, typename K>
void visit_descFld(const K &inVar, K &retVal) {
  // List of unsupported types by this function
  static_assert(!std::is_same_v<T, boost::rational<int>>);
  static_assert(!std::is_same_v<T, std::pair<int, int>>);
  static_assert(!std::is_same_v<T, std::pair<std::string, int>>);

  if constexpr (std::is_same_v<K, rdb::descFldVT>) {
    std::visit(Overload{
                   [&retVal](std::monostate) { retVal = T{}; },                                                 //
                   [&retVal](uint8_t a) { retVal = static_cast<T>(a); },                                        //
                   [&retVal](int a) { retVal = static_cast<T>(a); },                                            //
                   [&retVal](unsigned a) { retVal = static_cast<T>(a); },                                       //
                   [&retVal](boost::rational<int> a) { retVal = boost::rational_cast<T>(a); },                  //
                   [&retVal](float a) { retVal = narrowFloatTo<T>(a); },                                        //
                   [&retVal](double a) { retVal = narrowFloatTo<T>(a); },                                       //
                   [&retVal](std::pair<int, int> a) { SPDLOG_ERROR("TODO - pair-int->T"); },                    //
                   [&retVal](const std::pair<std::string, int> &a) { SPDLOG_ERROR("TODO - idxpair-int->T"); },  //
                   [&retVal](const std::string &a) { parse_string<T>(a, retVal); }                              //
               },
               inVar);
  } else {
    if (inVar.type() == typeid(std::monostate)) {
      retVal = T{};
    } else if (inVar.type() == typeid(uint8_t)) {
      retVal = static_cast<T>(std::any_cast<uint8_t>(inVar));
    } else if (inVar.type() == typeid(int)) {
      retVal = static_cast<T>(std::any_cast<int>(inVar));
    } else if (inVar.type() == typeid(unsigned)) {
      retVal = static_cast<T>(std::any_cast<unsigned>(inVar));
    } else if (inVar.type() == typeid(boost::rational<int>)) {
      retVal = boost::rational_cast<T>(std::any_cast<boost::rational<int>>(inVar));
    } else if (inVar.type() == typeid(float)) {
      retVal = narrowFloatTo<T>(std::any_cast<float>(inVar));
    } else if (inVar.type() == typeid(double)) {
      retVal = narrowFloatTo<T>(std::any_cast<double>(inVar));
    } else if (inVar.type() == typeid(std::pair<int, int>)) {
      SPDLOG_ERROR("No cast INTPAIR to any type here");
      retVal = static_cast<T>(0);
    } else if (inVar.type() == typeid(std::pair<std::string, int>)) {
      SPDLOG_ERROR("No cast IDXPAIR to any type here");
      retVal = static_cast<T>(0);
    } else if (inVar.type() == typeid(std::string)) {
      parse_string<T>(std::any_cast<std::string>(inVar), retVal);
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
  T retVal{};

  if (reqType == rdb::NULLTYPE) {
    if constexpr (std::is_same_v<T, rdb::descFldVT>) {
      return std::monostate{};
    } else if constexpr (std::is_same_v<T, std::any>) {
      return std::any(std::monostate{});
    } else {
      return T{};
    }
  }

  if constexpr (std::is_same_v<T, rdb::descFldVT>) {
    if (std::holds_alternative<std::monostate>(inVar)) return nullFallbackValue(reqType);
  } else if constexpr (std::is_same_v<T, std::any>) {
    if (inVar.type() == typeid(std::monostate)) {
      T fallback;
      std::visit([&fallback](const auto &v) { fallback = std::any(v); }, nullFallbackValue(reqType));
      return fallback;
    }
  }

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
    case rdb::NULLTYPE:
      break;
    case rdb::IDXPAIR:
      FatalError("convertTypes: IDXPAIR->T conversion not implemented");
      break;
    case rdb::INTPAIR:
      // Requested type is INT PAIR
      if constexpr (std::is_same_v<T, rdb::descFldVT>) {
        std::visit(Overload{                                                                                                 //
                            [&retVal](std::monostate) { retVal = std::make_pair(0, 0); },                                    //
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
                            },                                                 //
                            [&retVal](std::pair<int, int> a) { retVal = a; },  //
                            [&retVal](const std::pair<std::string, int> &a) {
                              retVal = std::make_pair(atoi(a.first.c_str()), a.second);
                            },  //
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
                            [&retVal](std::monostate) { retVal = boost::rational<int>(0, 1); },             //
                            [&retVal](uint8_t a) { retVal = boost::rational<int>(a); },                     //
                            [&retVal](int a) { retVal = boost::rational<int>(a); },                         //
                            [&retVal](unsigned a) { retVal = boost::rational<int>(static_cast<int>(a)); },  //
                            [&retVal](boost::rational<int> a) { retVal = a; },                              //
                            [&retVal](float a) { retVal = Rationalize(static_cast<double>(a)); },           //
                            [&retVal](double a) { retVal = Rationalize(a); },                               //
                            [&retVal](std::pair<int, int> a) {
                              if (a.second == 0) FatalError("convertTypes: rational denominator is zero (pair<int,int>)");
                              retVal = boost::rational<int>(a.first, a.second);
                            },  //
                            [&retVal](const std::pair<std::string, int> &a) {
                              retVal = boost::rational<int>(a.second, 1);
                            },  //  first is skipped
                            [&retVal](const std::string &a) {
                              std::istringstream in(a);
                              int nom{0};
                              int den{1};
                              in >> nom >> expect<'/'> >> den;
                              if (den == 0) FatalError("convertTypes: rational denominator is zero (string parse)");
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
          auto pairVar = std::any_cast<std::pair<int, int>>(inVar);
          if (pairVar.second == 0) FatalError("convertTypes: rational denominator is zero (any pair<int,int>)");
          retVal = boost::rational<int>(pairVar.first, pairVar.second);
        } else if (inVar.type() == typeid(std::string)) {
          std::istringstream in(std::any_cast<std::string>(inVar));
          int nom{0};
          int den{1};
          in >> nom >> expect<'/'> >> den;
          if (den == 0) FatalError("convertTypes: rational denominator is zero (any string parse)");
          retVal = boost::rational<int>(nom, den);
        }
      }
      break;
    case rdb::STRING:
      // Requested type is STRING
      if constexpr (std::is_same_v<T, rdb::descFldVT>) {
        std::visit(
            Overload{                                                                                                          //
                     [&retVal](std::monostate) { retVal = std::string(""); },                                                  //
                     [&retVal](uint8_t a) { retVal = std::to_string(a); },                                                     //
                     [&retVal](int a) { retVal = std::to_string(a); },                                                         //
                     [&retVal](unsigned a) { retVal = std::to_string(a); },                                                    //
                     [&retVal](float a) { retVal = std::to_string(a); },                                                       //
                     [&retVal](double a) { retVal = std::to_string(a); },                                                      //
                     [&retVal](std::pair<int, int> a) { retVal = std::to_string(a.first) + "," + std::to_string(a.second); },  //
                     [&retVal](const std::pair<std::string, int> &a) { retVal = a.first + "," + std::to_string(a.second); },   //
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
          auto pairVar = std::any_cast<std::pair<int, int>>(inVar);
          retVal       = std::to_string(pairVar.first) + "," + std::to_string(pairVar.second);
        } else if (inVar.type() == typeid(std::pair<std::string, int>)) {
          auto pairVar = std::any_cast<std::pair<std::string, int>>(inVar);
          retVal       = pairVar.first + "," + std::to_string(pairVar.second);
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
  double startx = inValue;
  double diff;
  double err1;
  double err2;
  int ttl = ttl_const;
  unsigned int val;
  for (;;) {
    val = static_cast<unsigned int>(startx);
    st.push(static_cast<int>(val));
    if ((ttl--) == 0) break;
    diff = startx - val;
    if (diff < DIFF) break;
    startx = 1 / diff;
    if (startx > (1 / DIFF)) break;
  }
  if (st.empty()) return {0, 1};
  boost::rational<int> result1(0, 1);
  boost::rational<int> result2(0, 1);
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

rdb::descFldVT nullFallbackValue(rdb::descFld type) {
  switch (type) {
    case rdb::BYTE:
      return uint8_t(0);
    case rdb::INTEGER:
      return 0;
    case rdb::UINT:
      return unsigned(0);
    case rdb::RATIONAL:
      return boost::rational<int>(0, 1);
    case rdb::FLOAT:
      return float(0);
    case rdb::DOUBLE:
      return double(0);
    case rdb::INTPAIR:
      return std::make_pair(0, 0);
    case rdb::IDXPAIR:
      return std::make_pair(std::string(""), 0);
    case rdb::STRING:
      return std::string("");
    case rdb::NULLTYPE:
      return std::monostate{};
    default:
      return uint8_t(0);
  }
}

// based: https://stackoverflow.com/questions/61182946/convert-stdany-to-stdvariant
rdb::descFldVT any_to_variant_cast(std::any a) {
  if (!a.has_value()) return std::monostate{};
  cast<rdb::descFldVT> castRI;
  if (a.type() == typeid(std::monostate)) return std::monostate{};
  if (a.type() == typeid(std::string)) return castRI(std::any_cast<std::string>(a), rdb::STRING);
  if (a.type() == typeid(int)) return castRI(std::any_cast<int>(a), rdb::INTEGER);
  if (a.type() == typeid(uint8_t)) return castRI(std::any_cast<uint8_t>(a), rdb::BYTE);
  if (a.type() == typeid(unsigned)) return castRI(std::any_cast<unsigned>(a), rdb::UINT);
  if (a.type() == typeid(double)) return castRI(std::any_cast<double>(a), rdb::DOUBLE);
  if (a.type() == typeid(float)) return castRI(std::any_cast<float>(a), rdb::FLOAT);
  if (a.type() == typeid(boost::rational<int>)) return castRI(std::any_cast<boost::rational<int>>(a), rdb::RATIONAL);
  throw std::bad_any_cast();
  return std::monostate{};  // Proforma
}

template struct cast<rdb::descFldVT>;
template struct cast<std::any>;
