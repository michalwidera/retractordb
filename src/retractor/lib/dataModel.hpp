#pragma once

#include <cstdint>
#include <map>
#include <memory>  // unique_ptr
#include <span>
#include <vector>

#include <boost/rational.hpp>

#include "fatalError.hpp"      // FatalError w kontroli krzyzowej handleAt() (tylko Debug)
#include "streamInstance.hpp"  // streamInstance (transitively includes qTree.hpp, rdb/payload.hpp)
class dataModel {
 private:
  qTree &coreInstance_;

  std::map<std::string, std::string> directive_{{":STORAGE", ""}, {":SUBSTRAT", ""}, {":ROTATION", ""}};

  /// Instancja wykonawcza strumienia po nazwie, ktora MOZE nie istniec w modelu.
  ///
  /// Sciezka IPC (`xqry -d`, `xqry -t`) pyta o strumienie wypisane z planu. Do 2026-09-25 plan
  /// i model potrafily sie rozjechac: getAdHoc() wnosil wezly do zywego drzewa przez
  /// importFrom(), a porazka rejestracji w modelu nie miala wycofania; dzis getAdHoc() przywraca
  /// plan przy kazdej porazce po imporcie. `qSet[id]` na nazwie spoza modelu nie zglaszalo
  /// bledu, tylko WSTAWIALO pusty unique_ptr i zaraz go luskalo -- czyli SIGSEGV. Brak nazwy
  /// jest tu bledem zgloszonym tak samo jak w qTree::getQuery: wyjatkiem, ktory handler
  /// zamienia w `error.response` dla klienta.
  [[nodiscard]] streamInstance &streamRuntime(const std::string &instance);

  [[nodiscard]] bool forwardRecordAvailable(const std::string &instance, int forwardIndex) const;
  [[nodiscard]] bool queryInputsAvailable(const query &qry, int logicalIndex);
  void bootstrapDeclaration(const query &qry);

  /// Instancje wykonawcze ULOZONE JAK PLAN: handles_[i] obsluguje coreInstance_.at(i). Tablica
  /// jest AKCELERATOREM, nie zmiana semantyki - powstaje z tych samych wywolan streamRuntime(),
  /// ktore stalyby w slocie, tyle ze raz na zmiane planu zamiast raz na takt. Wezel planu bez
  /// wpisu w modelu konczy sie tu tym samym zgloszonym bledem co dotad, tylko o kilka slotow
  /// wczesniej (przy pierwszej przebudowie po zmianie planu, a nie przy pierwszym slocie
  /// naleznym dla tego wezla).
  ///
  /// Wskazniki sa trwale: qSet trzyma unique_ptr, wiec adres instancji nie zalezy od wezla mapy,
  /// a z qSet nic sie nie usuwa przez cale zycie modelu.
  ///
  /// Blizniacze nazwy w planie sa stanem DOZWOLONYM (validateSubstratNameUniqueness pyta o
  /// rownosc programu, nie o unikalnosc nazwy). Tablica jest po POZYCJI, wiec blizniaki dostaja
  /// dwa wpisy o tym samym wskazniku - dokladnie tak, jak dzis oba wywolania streamRuntime(q.id)
  /// trafiaja w ten sam wpis qSet.
  std::vector<streamInstance *> handles_;

  /// Rewizja planu, dla ktorej zbudowano handles_. Zero znaczy "jeszcze nic", wiec nie trafia
  /// w zadne zywe drzewo - patrz qTree::planRevision().
  std::uint64_t handlesRevision_{0};

  void refreshStreamHandles();

  /// Uchwyt instancji stojacej na POZYCJI planu. W Debug sprawdza sie krzyzowo z wyszukaniem po
  /// nazwie - i to sprawdzenie ma prawo sie czerwienic: lapie zarowno przeterminowana tablice,
  /// jak i zmiane `query::id` w miejscu, ktora jest jedyna droga zmiany ksztaltu omijajaca
  /// rewizje planu (patrz GRANICA przy qTree::planRevision()). W Release znika w calosci - to
  /// wlasnie jego koszt mial zostac zdjety ze slotu.
  [[nodiscard]] streamInstance &handleAt(std::size_t position, [[maybe_unused]] const query &qry) {
#ifndef NDEBUG
    if (position >= handles_.size())
      FatalError("dataModel::processRows: stream handle table has {} entries, plan position is {}", handles_.size(), position);
    if (handles_[position] != &streamRuntime(qry.id))
      FatalError("dataModel::processRows: stream handle at position {} does not match plan node '{}'", position, qry.id);
#endif
    return *handles_[position];
  }

 public:
  std::map<std::string, std::unique_ptr<streamInstance>> qSet;

  explicit dataModel(qTree &coreInstance);
  ~dataModel();

  dataModel() = delete;

  /// Dolacza instancje wezlow planu o podanych nazwach - wszystkie albo zadnej. Zwraca nazwe,
  /// ktorej nie da sie dolaczyc (pusty napis = sukces); wyjatek z budowy instancji wychodzi
  /// przy nietknietym qSet. Wolajacy moze wiec wycofac plan, nie wyjmujac niczego z qSet - a
  /// z qSet nic sie nie usuwa (patrz handles_).
  [[nodiscard]] std::string addQueriesToModel(const std::vector<std::string> &ids);
  void syncDeclaredCapacities();

  std::unique_ptr<rdb::payload>::pointer getPayload(const std::string &instance,  //
                                                    int revOffset = 0);

  /*
   * Rekord strumienia po indeksie POSTĘPUJĄCYM (0-bazowym) na osi czasu źródła -
   * używane przez przeplot (#) i rozplot (&, %), których formuły (SOperations.hpp)
   * zwracają indeksy postępujące. Indeks spoza dostępnego zakresu (przyszłość,
   * poza pojemnością historii) daje rekord all-null.
   */
  rdb::payload fetchForward(const std::string &instance, int forwardIndex);

  /*
   * This function creates Input payload for ConstructOutputPayload data source
   * function need to be here because it access different streams from qSet
   *
   * Wezel planu i jego instancja przychodza OD WOLAJACEGO. Wolajacy (processRows) ma juz oba
   * pod reka, wiec szukanie ich tutaj po nazwie - `coreInstance_[instance]` skanem po planie
   * i `qSet[instance]` w mapie kluczowanej napisem - bylo powtorzeniem pracy juz wykonanej.
   * ZRODLA nadal adresuje nazwa i tak ma zostac: wezly jezdza przez kopie planu i przezywaja
   * do innego drzewa, wiec zapamietany uchwyt zrodla wskazywalby strukture, ktorej juz nie ma.
   */
  void constructInputPayload(const query &qry, streamInstance &runtime);

  /*
   * Wylicza okna rekordowe (MIN(pole:W:H) i rodzeństwo) dla jednego taktu strumienia.
   * Musi stać TU, a nie w streamInstance ani w ewaluatorze: okno czyta historię ŹRÓDŁA,
   * a dostęp do innych strumieni ma wyłącznie dataModel (qSet).
   */
  void computeWindowAggregates(const query &qry, streamInstance &runtime);

  /// Liczy jeden takt planu. Nalezne strumienie opisuje MASKA POZYCYJNA rownolegla do planu:
  /// dueMask[i] != 0 znaczy "element i planu jest nalezny w tym takcie". Dlugosc maski musi byc
  /// rowna dlugosci planu - pytanie brzmi zawsze "czy element i jest nalezny?", a nie "czy jest
  /// w zbiorze ta nazwa", wiec zbior napisow byl tu tylko kosztem: wezel drzewa i std::string na
  /// nalezny strumien w KAZDYM takcie, plus porownywanie napisow w trzech przebiegach ponizej.
  /// Maske trzyma i wypelnia executorsm::collectAwaitedStreams; uklad planu nie ma prawa zmienic
  /// sie miedzy jej wypelnieniem a tym wywolaniem.
  void processRows(std::span<const char> dueMask, const boost::rational<int> &currentTimeSlot = boost::rational<int>(0));
  void processZeroStep();

  std::vector<rdb::descFldVT> getRow(const std::string &instance, int timeOffset);

  size_t streamStoredSize(const std::string &instance);

  /** This function return length of data stream */
  size_t getStreamCount(const std::string &instance);

  /// Udaje, ze tablica uchwytow jest zbudowana dla BIEZACEJ rewizji planu, nie ruszajac jej
  /// zawartosci. Wytwarza dokladnie ten stan, ktorego niezmiennik zabrania: przeterminowana
  /// tablica wygladajaca na aktualna. Istnieje po to, zeby kontrola w handleAt() dala sie
  /// POKAZAC czerwona - sprawdzenie, ktore nigdy nie moze zawiesc, nie jest kontrola.
  void markHandlesFreshForUnitTest();

  /// @brief Nazwa pierwszego strumienia deklarowanego, którego źródło wyczerpało wejście.
  ///
  /// Pusty napis, gdy każde źródło ma jeszcze dane. Warunek stopu trybu --until-eof: przebieg
  /// trwa dopóty, dopóki KAŻDE wejście ma dane, bo od pierwszego wyczerpania rekordy liczyłyby
  /// się z all-null wstawionego za koniec pliku, a nie z danych.
  [[nodiscard]] std::string exhaustedInputStream() const;
};
