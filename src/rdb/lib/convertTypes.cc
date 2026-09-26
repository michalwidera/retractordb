#include "rdb/convertTypes.hpp"

#include <spdlog/spdlog.h>

#include "fatalError.hpp"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <istream>
#include <limits>
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
/// Regula: wartosc, ktorej typ docelowy nie pomiesci (takze NaN i nieskonczonosc), daje NULL
/// - ta sama regula co przepelnienie arytmetyki (checkedArith) i niefinitywny wynik `^`.
/// Nasycenie tez byloby okreslone, ale oddawaloby liczbe, ktorej nikt nie policzyl: 1e30 jako
/// INT_MAX, 300.0 w polu BYTE jako 255. Decyzja 2026-09-19, przeglad portu na macOS.
///
/// NULL wychodzi stad jako std::monostate, tak jak z parse_string ponizej. Dochodzi wiec do
/// `to_integer` i funkcji matematycznych nad typami calkowitymi (callFun), a zapis do rekordu
/// (payload::setItem / setItemVT) zamienia go na bit w nullBitset.
///
/// Zakres sprawdza sie na wartosci OBCIETEJ, bo tak konwertuje jezyk: 2147483647.5 miesci
/// sie w int (daje INT_MAX), 2147483648.0 juz nie. Gorna granica to 2^digits, potega dwojki,
/// wiec zapisuje sie dokladnie w float i double - inaczej niz `max()`, ktore we float
/// zaokragla sie do 2^31, czyli juz poza zakres int. Dolna granica (-2^31 albo 0) jest
/// dokladna z tego samego powodu. NaN nie spelnia zadnego porownania i wypada jako NULL.
template <typename T, typename F, typename K>
static void narrowFloatTo(F value, K &retVal) {
  static_assert(std::is_floating_point_v<F>);

  if constexpr (std::is_floating_point_v<T>) {
    retVal = static_cast<T>(value);
  } else {
    const F truncated      = std::trunc(value);
    const F upperExclusive = std::ldexp(F{1}, std::numeric_limits<T>::digits);
    if (truncated >= static_cast<F>(std::numeric_limits<T>::lowest()) && truncated < upperExclusive)
      retVal = static_cast<T>(value);
    else
      retVal = std::monostate{};
  }
}

/// Zwezenie ZMIENNOPRZECINKOWE -> WYMIERNE, ta sama regula zakresu co w narrowFloatTo wyzej.
///
/// Rationalize jest funkcja totalna i dla wejscia bez przyblizenia wymiernego oddaje {0,1}.
/// Zero, ktorego nikt nie policzyl, jest jednak dokladnie tym, co komentarz wyzej odrzuca
/// przy nasyceniu, dlatego NULL stoi na sciezce konwersji: NaN i nieskonczonosc nie maja
/// reprezentacji w `boost::rational<int>` i ida jako std::monostate, tak samo jak wartosc
/// zmiennoprzecinkowa poza zakresem typu calkowitego idzie nim z narrowFloatTo.
template <typename K>
static void rationalizeTo(double value, K &retVal) {
  if (std::isfinite(value))
    retVal = Rationalize(value);
  else
    retVal = std::monostate{};
}

/// Jak rationalizeTo, tylko wynik rozklada sie na pare licznik/mianownik (INTPAIR).
template <typename K>
static void rationalizePairTo(double value, K &retVal) {
  if (std::isfinite(value)) {
    const auto r = Rationalize(value);
    retVal       = std::make_pair(r.numerator(), r.denominator());
  } else {
    retVal = std::monostate{};
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
                   [&retVal](float a) { narrowFloatTo<T>(a, retVal); },                                         //
                   [&retVal](double a) { narrowFloatTo<T>(a, retVal); },                                        //
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
      narrowFloatTo<T>(std::any_cast<float>(inVar), retVal);
    } else if (inVar.type() == typeid(double)) {
      narrowFloatTo<T>(std::any_cast<double>(inVar), retVal);
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
                            [&retVal](float a) { rationalizePairTo(static_cast<double>(a), retVal); },                       //
                            [&retVal](double a) { rationalizePairTo(a, retVal); },                                           //
                            [&retVal](std::pair<int, int> a) { retVal = a; },                                                //
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
          rationalizePairTo(std::any_cast<float>(inVar), retVal);
        } else if (inVar.type() == typeid(double)) {
          rationalizePairTo(std::any_cast<double>(inVar), retVal);
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
                            [&retVal](float a) { rationalizeTo(static_cast<double>(a), retVal); },          //
                            [&retVal](double a) { rationalizeTo(a, retVal); },                              //
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
          rationalizeTo(std::any_cast<float>(inVar), retVal);
        } else if (inVar.type() == typeid(double)) {
          rationalizeTo(std::any_cast<double>(inVar), retVal);
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

/// Znak wynosi sie PRZED petle ulamka lancuchowego, a konwersje w petli oslania ten sam
/// sprawdzony zakres co narrowFloatTo wyzej.
///
/// `static_cast<unsigned int>(-2.5)` jest zachowaniem NIEOKRESLONYM i procesory rozstrzygaja
/// je ROZNIE: x86-64 (cvttsd2si) bierze mlodsze 32 bity i oddaje 4294967294, arm64 (fcvtzu)
/// NASYCA do 0. Ta sama baza dawala wiec `-2/1` na jednej maszynie i `0/1` na drugiej, po cichu
/// i bez bledu. Na obu bylo to zreszta zle: `diff = startx - val` liczylo sie na wartosci BEZ
/// ZNAKU, wiec dla ujemnego wejscia petla urywala sie po pierwszej cyfrze i ulamek nigdy nie
/// powstawal - `-2.5` dawalo `-2/1`, nie `-5/2`.
///
/// Ulamek lancuchowy jest symetryczny wzgledem znaku: rozwiniecie |x| daje p/q, a -|x| daje
/// -p/q. Liczymy wiec na wartosci bezwzglednej i negujemy wynik. `startx` jest wtedy zawsze
/// nieujemne, wiec z zakresu zostaje sama GORNA granica - sprawdzana na wartosci OBCIETEJ,
/// dokladnie jak w narrowFloatTo, i z ta sama potega dwojki jako granica.
///
/// NaN nie spelnia zadnego porownania i wypada z petli tak samo jak nieskonczonosc i jak
/// wartosc za duza na `int`: zaden wyraz nie wchodzi do ulamka, a pusty ulamek oddaje {0,1}.
/// Funkcja jest totalna, bo zwraca `boost::rational<int>` przez wartosc i NULL-a nie ma czym
/// wyrazic; niefinitywne wejscie odsiewa rationalizeTo/rationalizePairTo, jeszcze przed wywolaniem.
///
/// Wynik to konwergent h/k liczony rekurencja h = a*h1 + h2, k = a*k1 + k2 w `int64_t`.
/// Wczesniej wyrazy szly na stos i skladaly sie od tylu na `boost::rational<int>`, ktory zakresu
/// nie pilnuje: konwergent ponad `int` to przepelnienie liczby ze znakiem, czyli zachowanie
/// nieokreslone, a w praktyce ulamek o zlej wartosci, czesto ujemny - interwal `0.333333` dawal
/// 2064120233/1923156540, a `3^(3/2)` nad RATIONAL -4.768 (#309). Konwergenty rosna monotonicznie,
/// wiec petla konczy sie na pierwszym, ktory nie miesci sie w `int`, i oddaje poprzedni - najlepsze
/// przyblizenie, jakie `boost::rational<int>` zapisze. Tam, gdzie skladanie sie nie przepelnialo,
/// wynik jest identyczny: to ten sam ulamek, a konwergent jest juz nieskracalny. Sama rekurencja
/// nie przepelnia sie: a < 2^31 i h1, k1 <= INT_MAX, wiec a*h1 + h2 < 2^63.
boost::rational<int> Rationalize(const double inValue, const double DIFF /*=1E-6*/, const int ttl_const /*=11*/) {
  const double upperExclusive = std::ldexp(1.0, std::numeric_limits<int>::digits);
  const std::int64_t limit    = std::numeric_limits<int>::max();
  // Start rekurencji: h_{-1}/k_{-1} = 1/0, h_{-2}/k_{-2} = 0/1.
  std::int64_t h1 = 1;
  std::int64_t h2 = 0;
  std::int64_t k1 = 0;
  std::int64_t k2 = 1;
  double startx   = std::fabs(inValue);
  int ttl         = ttl_const;
  for (;;) {
    const double truncated = std::trunc(startx);
    if (!(truncated < upperExclusive)) break;
    const auto a         = static_cast<std::int64_t>(truncated);
    const std::int64_t h = a * h1 + h2;
    const std::int64_t k = a * k1 + k2;
    if (h > limit || k > limit) break;
    h2 = h1;
    h1 = h;
    k2 = k1;
    k1 = k;
    if ((ttl--) == 0) break;
    const double diff = startx - truncated;
    if (diff < DIFF) break;
    startx = 1 / diff;
    if (startx > (1 / DIFF)) break;
  }
  if (k1 == 0) return {0, 1};  // zaden wyraz nie wszedl - k1 zostalo przy k_{-1}
  const boost::rational<int> result(static_cast<int>(h1), static_cast<int>(k1));
  return std::signbit(inValue) ? -result : result;
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
