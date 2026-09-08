#pragma once

#include <cstdint>
#include <list>
#include <string>
#include <utility>

#include "token.hpp"

struct rule {
  std::string name;
  std::list<token> condition;

  enum actionType : std::uint8_t { UNKNOWN_ACTION, DUMP, SYSTEM } action{UNKNOWN_ACTION};

  std::pair<long int, long int> dumpRange{std::make_pair(0, 0)};
  size_t dump_retention{0};

  std::string systemCommand;

  /// Liczba rekordow strumienia, od ktorej regula zaczyna byc oceniana. 0 — czyli regula
  /// z pliku planu — znaczy "od pierwszego rekordu".
  ///
  /// Niezerowa wartosc niesie granice historii reguly dolaczonej w locie (ad-hoc). Silnik
  /// nie wiedzial wczesniej, ze ma dla niej cokolwiek gromadzic, wiec ujemna czesc zakresu
  /// DUMP moze siegac wylacznie rekordow powstalych PO dolaczeniu — rekordy wczesniejsze sa
  /// logicznie niedostepne, nawet jesli nadal leza w magazynie. Regula czeka wiec nieuzbrojona,
  /// az historii po dolaczeniu uzbiera sie tyle, ile zada jej zakres; dopiero wtedy `dumpManager`
  /// siegajacy `abs(dumpRange.first)` rekordow wstecz nie ma jak trafic sprzed tej granicy.
  /// Zrzutow skroconych nie wystawiamy: zakres znaczy to samo, co znaczyl w planie.
  size_t armAtCount{0};

  rule(std::string name, std::list<token> condition) : name(std::move(name)), condition(std::move(condition)) {}
};
