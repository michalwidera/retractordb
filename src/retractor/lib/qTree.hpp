#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/rational.hpp>

#include "query.hpp"

/// Plan zapytan - wektor wezlow w kolejnosci przetwarzania, plus wyszukanie po nazwie.
///
/// Dziedziczenie jest PRYWATNE i to jest wlasnosc klasy, nie szczegol zapisu. Publiczne
/// wystawialo cala powierzchnie std::vector - w tym niejawna konwersje do
/// `std::vector<query> &`, ktora pozwalala podmienic zawartosc planu rzutowaniem na baze z
/// dowolnego miejsca w drzewie. Ponizej jest wystawione DOKLADNIE to, co drzewo wola;
/// wszystko inne (front, back, data, insert, emplace, resize, reserve, swap, rbegin,
/// operator==) bylo dostepne i nieuzywane.
///
/// Zwezenie jest tez warunkiem kazdego przyszlego niezmiennika nad planem: zbior operacji
/// MUTUJACYCH jest teraz wyliczalny (push_back, pop_back, erase, clear, replaceAll,
/// sort, topologicalSort), wiec da sie go domknac - a nie tylko miec nadzieje, ze nikt nie
/// siegnal do bazy bokiem.
class qTree : private std::vector<query> {
  /* Topological sort vars */
  std::map<std::string, bool> visited_;
  std::map<std::string, std::vector<std::string>> adj_;  // adjacency list of graph
  std::vector<std::string> ans_;

  void dfs(const std::string &v);  // Depth First Traversal
  int getSeqNr(const std::string &query_name);

  /// Kolejny numer z dozownika procesu - patrz planRevision() nizej. Atomowy, bo kompilacja
  /// kopii planu w kanale ad-hoc biegnie na watku komunikacyjnym rownolegle ze slotem:
  /// obiekty sa wtedy rozne, ale dozownik jeden.
  static std::uint64_t nextPlanRevision();

  std::uint64_t planRevision_{nextPlanRevision()};

 public:
  // Powierzchnia wektora wystawiona jawnie - lista uzywana przez drzewo, nic ponadto.
  // Odczyt:
  using std::vector<query>::at;
  using std::vector<query>::size;
  using std::vector<query>::empty;
  using std::vector<query>::begin;
  using std::vector<query>::end;
  // Mutacje - komplet punktow, w ktorych zmienia sie KSZTALT planu (obok replaceAll,
  // sort i topologicalSort nizej). Kazda bierze nowy numer rewizji, wiec zadna nie moze
  // zostac `using`-iem: deklaracja using wpuszcza metode bazy nietknieta i niczego by nie
  // podbila.
  void push_back(const query &node);
  void push_back(query &&node);
  void pop_back();
  std::vector<query>::iterator erase(std::vector<query>::const_iterator position);
  std::vector<query>::iterator erase(std::vector<query>::const_iterator first, std::vector<query>::const_iterator last);
  void clear();

  // Wlasne operator[](nazwa) UKRYWA komplet przeciazen bazy, wiec bez tej deklaracji dostep po
  // pozycji - plan[i] - nie kompiluje sie wcale, a komunikat wskazuje na std::string zamiast na
  // przyczyne (w C++17 gorzej: plan[0] szlo przez 0 -> const char* -> std::string i wywracalo
  // sie w locie; dzis basic_string(nullptr_t) jest = delete). Rozstrzyganie jest jednoznaczne:
  // z typu calkowitego nie ma niejawnej konwersji do std::string, wiec indeks zawsze wybiera
  // wersje pozycyjna, a napis - wersje po nazwie.
  using std::vector<query>::operator[];

  query &operator[](const std::string &query_name) { return getQuery(query_name); };

  query &getQuery(const std::string &query_name);

  /// Podmienia CALA zawartosc planu, zostawiajac pola skladowe klasy nietkniete.
  ///
  /// Stoi tu zamiast `static_cast<std::vector<query> &>(plan) = ...` w miejscu wywolania z
  /// dwoch powodow. Pierwszy: rzutowanie na baze wymaga dziedziczenia publicznego, czyli
  /// calej reszty powierzchni wektora przy okazji. Drugi jest starszy - przypisanie calego
  /// qTree (plan = tempInstance) nadpisywalo takze skladowe, w tym maxCapacity, wartosciami
  /// domyslnymi; bylo to nieszkodliwe dopoki sortowanie wywolywano wylacznie przed
  /// computeRequiredCapacities(). Nazwana metoda mowi wprost, ze podmieniane sa WEZLY, a nie
  /// stan klasy.
  void replaceAll(std::vector<query> &&nodes);

  // Keep explicit comparator: std::ranges::less is not invocable for query on newer GCC.
  void sort() {
    std::ranges::sort(*this, [](const query &lhs, const query &rhs) { return lhs < rhs; });
    planRevision_ = nextPlanRevision();
  };
  void topologicalSort();
  bool exists(const std::string &query_name);

  boost::rational<int> getDelta(const std::string &query_name);
  void dumpCore();
  std::set<boost::rational<int>> getAvailableTimeIntervals();

  std::map<std::string, int> maxCapacity;

  /// Numer KSZTALTU planu: jego dlugosci, kolejnosci wezlow i nazwy na kazdej pozycji. Sluzy
  /// temu, kto trzyma strukture rownolegla do planu (dataModel::handles_) i musi wiedziec,
  /// kiedy ja przebudowac. Porownuje sie go przez `!=`, nie przez `<`.
  ///
  /// Numery wydaje JEDEN dozownik na proces, a nie licznik na obiekt, i stad plynie wlasnosc,
  /// ktora tu obowiazuje: ROWNY NUMER ZNACZY ROWNY KSZTALT, takze miedzy roznymi drzewami.
  /// Dwie drogi, ktore przy liczniku na obiekt trzeba by obsluzyc z reki, zalatwia sama:
  ///  * kopia planu (`qTree coreInstanceCopy = *coreInstancePtr` w executorsm::getAdHoc)
  ///    niesie numer zrodla, bo w tej chwili ma dokladnie jego ksztalt; pierwsza mutacja
  ///    kopii bierze nowy numer, wiec dwa drzewa o roznym ksztalcie nigdy go nie dziela;
  ///  * przypisanie calego drzewa (`*coreInstancePtr = qTree{}` przy przeladowaniu planu)
  ///    wnosi numer przypisywanego drzewa, a ten dopiero co wyszedl z dozownika - jest wiec
  ///    rozny od wszystkiego, co obserwator mogl zapamietac. Licznik na obiekt wniosilby tu
  ///    wartosc NIZSZA i przeterminowana struktura wygladalaby na aktualna.
  ///
  /// GRANICA. Numer opisuje ksztalt, a nie tresc wezla: at(), operator[] i getQuery() wydaja
  /// query&, wiec zmiana pola w miejscu numeru nie rusza. Dla `isOneShot` czy `lRules` tak ma
  /// byc; dla `id` byloby to cicha pomylka adresu. Dzis zadne miejsce nie przypisuje do `id`
  /// wezla JUZ stojacego w drzewie - kazde `.id =` w src/ pisze do lokalnego `query` przed
  /// wniesieniem go tutaj, takze rozwiniecie generatora strumieni w compiler.cpp, ktore
  /// publikuje gotowy plan przez replaceAll(). To jest WARUNEK, nie obserwacja: gdyby takie
  /// przypisanie kiedykolwiek powstalo, musi wziac nowy numer.
  [[nodiscard]] std::uint64_t planRevision() const { return planRevision_; }
};
