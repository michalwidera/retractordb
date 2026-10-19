#pragma once

#include <cstdint>
#include <map>
#include <memory>  // unique_ptr
#include <span>
#include <vector>

#include <fmt/format.h>
#include <boost/rational.hpp>

#include "rdb/error.hpp"       // RDB_ASSERT w kontroli krzyzowej handleAt() (tylko Debug), rdb::Result
#include "streamInstance.hpp"  // streamInstance (transitively includes qTree.hpp, rdb/payload.hpp)
/// Obiekt klasy dataModel powinien:
/// - [datamodel.data-model.1~1] w create zapamiętywać wartości dyrektyw :STORAGE, :SUBSTRAT i :ROTATION, usuwać dyrektywy z planu i tworzyć instancję każdego pozostałego strumienia z otwartym magazynem w katalogu :STORAGE i we wskazanym MemoryStore, a błąd otwarcia magazynu zwracać jako Result z nazwą strumienia.
///   Kontrola strukturalna: wartości :SUBSTRAT i :ROTATION są tylko zapamiętywane w directive_ i nie mają obserwowalnego skutku w dataModel.
/// - [datamodel.data-model.2~1] traktować pusty plan i dyrektywę z pustą wartością jako naruszenie niezmiennika.
/// - [datamodel.data-model.3~1] w addQueriesToModel dołączać instancje wszystkich wskazanych węzłów planu albo żadnej, zwracając nazwę, której nie da się dołączyć, i nie usuwając z modelu żadnej istniejącej instancji.
/// - [datamodel.data-model.4~1] w processRows liczyć wyłącznie węzły oznaczone w masce pozycyjnej o długości planu, w kolejności: uzbrojenie nowych źródeł i odczyt rekordów DEVICE, obliczenie strumieni niebędących deklaracjami w porządku planu, a na końcu przesunięcie pozostałych źródeł deklarowanych do następnego rekordu.
///   Zakres testow: `DataModelSlotContract.processRows_computes_due_nodes_in_plan_order_before_advancing_sources`, `xschema.getRow_1`, `it_device_timeout-run` (rekord DEVICE na początku slotu).
/// - [datamodel.data-model.5~1] nie emitować rekordu strumienia w pierwszych origin + ogon należnych slotach, ani jako zera, ani jako NULL.
/// - [datamodel.data-model.6~1] dla strumienia dołączonego ad hoc wyznaczać indeks pierwszego rekordu z bieżącego slotu i emitować go dopiero, gdy wszystkie jego wejścia są dostępne.
///   Zakres testow: `DataModelSlotContract.adhoc_stream_starts_at_the_index_of_its_first_due_slot`, `it_issue227_join_alignment-adhoc-origin` (także oczekiwanie na wejścia źródła dołączonego ad hoc).
/// - [datamodel.data-model.7~1] dla każdego emitowanego strumienia budować rekord wejściowy z klauzuli FROM, liczyć okna rekordowe, budować rekord wyjściowy z listy SELECT, zapisywać go do magazynu i wykonywać reguły strumienia.
/// - [datamodel.data-model.8~1] przerywać slot na pierwszym błędzie odczytu, zapisu albo ewaluacji i zwracać go jako Result bez kończenia procesu.
///   Zakres testow: `DataModelSlotContract.processRows_stops_the_slot_at_the_first_error`, `it_env_error_exit-run` (błąd środowiska w slocie przez hak RDB_FAULT_ENV_IN_SLOT).
/// - [datamodel.data-model.9~1] przez fetchForward zwracać rekord strumienia po indeksie logicznym, a dla indeksu spoza dostępnej historii rekord all-null, ważny do następnego odczytu tego strumienia.
///   Kontrola strukturalna: ważność wskaźnika do następnego odczytu wynika z odczytu przez payload magazynu albo unavailableRecord (fetchForward w dataModel.cpp).
/// - [datamodel.data-model.10~1] przez getPayload udostępniać payload wraz z informacją NULL: dla strumienia SELECT rekord cofnięty o podaną liczbę rekordów albo all-null przy braku rekordu, a dla deklaracji payload bieżący niezależnie od przesunięcia.
/// - [datamodel.data-model.11~1] przez getRow zwracać wartości pól rekordu cofniętego o podaną liczbę rekordów dla strumienia SELECT albo rekordu bieżącego deklaracji niezależnie od przesunięcia, zastępując wartości NULL przez nullFallbackValue typu pola, także przy braku rekordu SELECT.
/// - [datamodel.data-model.12~1] przez getStreamCount i streamStoredSize zwracać liczbę zapisanych rekordów strumienia i jej rozmiar w bajtach.
/// - [datamodel.data-model.13~1] przez syncDeclaredCapacities zwiększać pojemność historii źródeł deklarowanych do wymaganej przez plan, nigdy jej nie zmniejszając.
/// - [datamodel.data-model.14~1] przez exhaustedInputStream zwracać nazwę pierwszego źródła deklarowanego, które wyczerpało wejście, albo pusty napis.
/// - [datamodel.data-model.15~1] traktować odwołanie do strumienia z planu bez instancji w modelu oraz maskę o długości innej niż plan jako naruszenie niezmiennika.
/// - [datamodel.data-model.16~1] przy create budować wartości bieżące zmiennych VAR z wartości deklaracji qTree::variables (D6, #531) i przekazywać je w processRows wyrażeniom pól i warunkom reguł każdego liczonego strumienia.
class dataModel {
 private:
  qTree &coreInstance_;

  /// Memory Store magazynow MEMORY tego modelu; nullptr = instancja domyslna procesu. Przekazywany
  /// kazdemu streamInstance, takze dodanemu ad hoc - patrz streamInstance.hpp.
  rdb::MemoryStore *memory_;

  std::map<std::string, std::string> directive_{{":STORAGE", ""}, {":SUBSTRAT", ""}, {":ROTATION", ""}};

  /// Instancja wykonawcza strumienia po nazwie, ktora MOZE nie istniec w modelu.
  ///
  /// Sciezka IPC (`xqry -d`, `xqry -t`) pyta o strumienie wypisane z planu. Do 2026-09-25 plan
  /// i model potrafily sie rozjechac: getAdHoc() wnosil wezly do zywego drzewa przez
  /// importFrom(), a porazka rejestracji w modelu nie miala wycofania; dzis getAdHoc() przywraca
  /// plan przy kazdej porazce po imporcie. `qSet[id]` na nazwie spoza modelu nie zglaszalo
  /// bledu, tylko WSTAWIALO pusty unique_ptr i zaraz go luskalo -- czyli SIGSEGV. Brak nazwy
  /// jest tu zlamanym niezmiennikiem (RDB_ASSERT), tak samo jak w qTree::getQuery: nazwy od
  /// klienta sprawdza wczesniej handler komendy (exists()) i odpowiada `error.response`.
  [[nodiscard]] streamInstance &streamRuntime(const std::string &instance);

  [[nodiscard]] bool forwardRecordAvailable(const std::string &instance, int forwardIndex) const;
  [[nodiscard]] bool queryInputsAvailable(const query &qry, int logicalIndex);
  [[nodiscard]] rdb::Result<> bootstrapDeclaration(const query &qry);
  /// Kolejny rekord uzbrojonej deklaracji: odczyt z akcesora i fire() do historii.
  [[nodiscard]] rdb::Result<> advanceDeclaration(const query &qry, rdb::storage &output);

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
    RDB_ASSERT(position < handles_.size(), "dataModel::processRows: stream handle table has {} entries, plan position is {}",
               handles_.size(), position);
    RDB_ASSERT(handles_[position] == &streamRuntime(qry.id),
               "dataModel::processRows: stream handle at position {} does not match plan node '{}'", position, qry.id);
#endif
    return *handles_[position];
  }

 public:
  std::map<std::string, std::unique_ptr<streamInstance>> qSet;

 private:
  dataModel(qTree &coreInstance, rdb::MemoryStore *memory);

  /// Wartosci biezace zmiennych VAR planu, indeksowane jak qTree::variables (#531). Etap A ich nie
  /// zmienia - zostaja wartosciami z deklaracji (D6).
  std::vector<rdb::descFldVT> variables_;

 public:
  /// Model planu: instancja kazdego strumienia z otwartym magazynem. Blad otwarcia magazynu wraca
  /// jako Result - zamiast rzutu z konstruktora. Dyrektywa z pusta wartoscia to zlamany niezmiennik
  /// (RDB_ASSERT): odrzuca ja juz parser ("requires a non-empty value").
  [[nodiscard]] static rdb::Result<std::unique_ptr<dataModel>> create(qTree &coreInstance, rdb::MemoryStore *memory = nullptr);
  ~dataModel();

  dataModel() = delete;

  /// Dolacza instancje wezlow planu o podanych nazwach - wszystkie albo zadnej. Zwraca nazwe,
  /// ktorej nie da sie dolaczyc (pusty napis = sukces); blad budowy instancji wraca
  /// przy nietknietym qSet. Wolajacy moze wiec wycofac plan, nie wyjmujac niczego z qSet - a
  /// z qSet nic sie nie usuwa (patrz handles_).
  [[nodiscard]] std::string addQueriesToModel(const std::vector<std::string> &ids);
  void syncDeclaredCapacities();

  [[nodiscard]] rdb::Result<std::unique_ptr<rdb::payload>::pointer> getPayload(const std::string &instance,  //
                                                                               int revOffset = 0);

  /*
   * Rekord strumienia po indeksie POSTĘPUJĄCYM (0-bazowym) na osi czasu źródła -
   * używane przez przeplot (#) i rozplot (&, %), których formuły (SOperations.hpp)
   * zwracają indeksy postępujące. Indeks spoza dostępnego zakresu (przyszłość,
   * poza pojemnością historii) daje rekord all-null.
   *
   * Referencja wskazuje wpis historii źródła, jego bieżący payload albo rekord all-null
   * (streamInstance::unavailableRecord) i jest ważna tylko do następnego odczytu tego
   * strumienia - kolejny revRead nadpisuje bieżący payload. Nie przechowywać jej.
   */
  [[nodiscard]] rdb::Result<const rdb::payload *> fetchForward(const std::string &instance, int forwardIndex);

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
  [[nodiscard]] rdb::Result<> constructInputPayload(const query &qry, streamInstance &runtime);

 private:
  /*
   * Wylicza okna rekordowe (MIN(pole:W:H) i rodzeństwo) dla jednego taktu strumienia.
   * Musi stać TU, a nie w streamInstance ani w ewaluatorze: okno czyta historię ŹRÓDŁA,
   * a dostęp do innych strumieni ma wyłącznie dataModel (qSet).
   */
  [[nodiscard]] rdb::Result<> computeWindowAggregates(const query &qry, streamInstance &runtime);

 public:
  /// Liczy jeden takt planu. Nalezne strumienie opisuje MASKA POZYCYJNA rownolegla do planu:
  /// dueMask[i] != 0 znaczy "element i planu jest nalezny w tym takcie". Dlugosc maski musi byc
  /// rowna dlugosci planu - pytanie brzmi zawsze "czy element i jest nalezny?", a nie "czy jest
  /// w zbiorze ta nazwa", wiec zbior napisow byl tu tylko kosztem: wezel drzewa i std::string na
  /// nalezny strumien w KAZDYM takcie, plus porownywanie napisow w trzech przebiegach ponizej.
  /// Maske trzyma i wypelnia executorsm::collectAwaitedStreams; uklad planu nie ma prawa zmienic
  /// sie miedzy jej wypelnieniem a tym wywolaniem.
  /// Jeden slot planu. Blad odczytu, zapisu albo ewaluacji konczy slot w miejscu, w ktorym
  /// wystapil, i wraca do wolajacego - demon zatrzymuje sie na nim (executorsm::run), silnik
  /// osadzony oddaje go z step().
  [[nodiscard]] rdb::Result<> processRows(std::span<const char> dueMask,
                                          const boost::rational<int> &currentTimeSlot = boost::rational<int>(0));
  [[nodiscard]] rdb::Result<> processZeroStep();

  [[nodiscard]] rdb::Result<std::vector<rdb::descFldVT>> getRow(const std::string &instance, int timeOffset);

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
