#pragma once

#include <array>
#include <memory>  // unique_ptr
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dumpManager.hpp"
#include "qTree.hpp"        // qTree
#include "rdb/storage.hpp"  // rdb::storage (transitively includes descriptor.hpp, payload.hpp)

/// @brief Klasa streamInstance umożliwia tworzenie zawartości poszczególnych rekordów
///
/// Obiekt klasy streamInstance powinien:
/// - być inicjalizowany z instancją qTree i query, na podstawie których będzie tworzył zawartość rekordów (payload).
/// - w trakcie inicjalizacji powinna być możliwość dodania dodatkowego parametru, np. ścieżki do katalogu, w którym będą przechowywane dane (storagePathParam).
/// - obsługiwać redukcje liczbowych pól bieżącego rekordu STREAM_MIN (MIN), STREAM_MAX (MAX), STREAM_SUM (SUMC)
///   i STREAM_AVG (AVG), obejmujące wszystkie sloty płaskie, w tym elementy tablic, z pominięciem null;
///   zwracać minimum, maksimum, sumę lub sumę podzieloną przez liczbę wartości nie-null, a przy braku takich
///   wartości null; wynik dla BYTE, INTEGER, UINT i RATIONAL ma typ RATIONAL, dla FLOAT lub DOUBLE zachowuje typ,
/// - przez reduceRecordWindow() wyznaczać MIN, MAX, SUMC i AVG z jednej wartości pola group.slot albo
///   wyrażenia group.program na rekord, w zakresie lastLogicalIndex - (group.width - 1) do lastLogicalIndex;
///   pomijać null i niedostępne rekordy, liczyć średnią z liczby pozostałych wartości, a przy zerowej liczbie
///   zwracać null dla wszystkich agregatów; typ wyniku group.valueType należy do RATIONAL, FLOAT i DOUBLE,
/// - przez constructAgsePayload() budować okno długości abs(length) ze spłaszczonych pozycji źródła
///   windowIndex * step - (abs(length) - 1) do windowIndex * step; dla length > 0 umieszczać najnowszy
///   element jako pierwszy, dla length < 0 zachowywać kolejność napływu; brakujące elementy oznaczać null,
/// - wyrażenia pól i warunki reguł wykonywać przez expressionEvaluator: +, -, *, /, ^, NEGATE, BIT_NOT (~),
///   porównania =, !=, <, >, <=, >=, AND, OR i NOT, wywołania CALL/CALL2 i odczyty pól; wyniki okien
///   udostępniać wyrażeniom pól przez windowValues; zakres typów operandów, nazw funkcji i konwersji
///   określa kontrakt expressionEvaluator,
/// - przez constructRulesAndUpdate() oceniać reguły qry.lRules na kopii bieżącego payloadu wyjściowego,
///   gdy getRecordsCount() >= armAtCount; warunek null, zero liczbowe lub pusty STRING nie uruchamia akcji,
///   niezerowa liczba albo niepusty STRING uruchamia akcję, a wynik INTPAIR/IDXPAIR zwraca Errc::Eval,
/// - dla prawdziwego warunku DUMP rejestrować w dumpManager zadanie z nazwą reguły, dumpRange i dump_retention;
///   dla SYSTEM wywoływać systemCommand przez ::system(), logując nieudane wywołanie lub zakończenie z błędem;
///   po ocenie reguł, także gdy lista jest pusta, wywoływać dumpManager::processStreamChunk(qry.id);
///   błędy ewaluacji, rejestracji zadania lub przetwarzania zrzutu propagować w Result,
/// - przez constructOutputPayload(fields) obliczać kolejne programy field.lProgram z inputPayload
///   i windowValues, wynik pozycji i konwertować przez cast<descFldVT> do outputPayload->descriptor[i].rtype
///   i zapisywać do slotu i przez setItemVT(); wynik null lub nieudana konwersja oznaczają null pola;
///   po udanym obliczeniu wszystkich pól zwracać sukces bez dopisywania rekordu do magazynu, co należy
///   do wywołującego; błąd przerywa obliczenia i wraca w Result, bez cofania wcześniej ustawionych pól.

struct streamInstance {
  qTree &coreInstance;
  streamInstance() = delete;

  std::unique_ptr<rdb::storage> outputPayload;  // here is payload that will be stored - select clause
  std::unique_ptr<rdb::payload> inputPayload;   // payload used for computation in select
                                                // clause - created by from clause.
  /// @brief Liczba slotów własnego interwału, które upłynęły od startu strumienia.
  ///
  /// Potrzebna, bo od czasu wprowadzenia ogona (query::startupLatency) liczba rekordów NIE jest już równa
  /// numerowi slotu: przez pierwsze startupLatency slotów strumień nie emituje niczego. Licznik rozdziela
  /// te dwie wielkości - outputPayload->getRecordsCount() pozostaje indeksem elementu, ten licznik jest
  /// pozycją na siatce slotów.
  size_t elapsedSlots = 0;

  /// @brief Indeks logiczny fizycznego rekordu 0 tej instancji.
  ///
  /// Plan uruchamiany od początku używa query::logicalOrigin. Instancja dodana ad hoc
  /// dostaje bazę dopiero w pierwszym należnym jej slocie bieżącej osi czasu; do tego
  /// momentu std::nullopt odróżnia ją od strumienia startowego oczekującego na origin/ogon.
  std::optional<int> logicalIndexBase;

  /// @brief Czy rekord skladnika przeplotu ma uklad slotu wejscia tej instancji: [0] lewy, [1] prawy.
  ///
  /// Wypelniane w pierwszym takcie, w ktorym przeplot bierze dana strone (dataModel::constructInputPayload).
  /// Odpowiedz zalezy tylko od deskryptora wyjscia skladnika i deskryptora wejscia tego wezla, a oba sa
  /// stale przez zycie instancji: qSet tylko przybywa i zadnej instancji nie podmienia. Poza wezlem
  /// przeplotu zostaje puste.
  std::array<std::optional<bool>, 2> hashSideMatchesInput;

  /// @brief Czy slot wejscia tej instancji ma dokladnie uklad sumy skladnikow `A+B`.
  ///
  /// Wypelniane w pierwszym takcie sumy (dataModel::constructInputPayload), z tego samego powodu co
  /// hashSideMatchesInput: deskryptory skladnikow i wejscia sa stale przez zycie instancji. Prawda
  /// pozwala pisac sume wprost do wejscia (payload::assignConcatenation). Poza wezlem sumy zostaje puste.
  std::optional<bool> addMatchesInput;

  /// @brief Rekord all-null tego strumienia, zwracany przez dataModel::fetchForward dla rekordu niedostepnego.
  ///
  /// Budowany przy pierwszym takim odczycie i potem tylko czytany. Trzymany tutaj, bo fetchForward
  /// zwraca referencje, a rekord niedostepny jest rzadki - nie ma po co budowac go w kazdym takcie.
  std::optional<rdb::payload> unavailableRecord;

  // This constructor will create data based on query
  //
  // `memory` to sklep magazynu MEMORY, ktory dostaje outputPayload; nullptr wybiera instancje
  // domyslna procesu (rdb::MemoryStore::processDefault), czyli dotychczasowe zachowanie demona.
  // Silnik osadzony (rdb::embed::Engine) podaje tu sklep wlasny - to jest miejsce, w ktorym
  // izolacja dwoch silnikow w jednym procesie dociera do planu, a nie tylko do magazynu.
  //
  // Budowa przez create(): magazyn wyniku, ktory sie nie otworzyl (albo jego .desc nie da sie
  // wczytac), wraca jako blad zamiast polowicznie zbudowanej instancji z napisem w polu.
  [[nodiscard]] static rdb::Result<std::unique_ptr<streamInstance>> create(qTree &coreInstance, query &qry,
                                                                           const std::string &storagePathParam = "",
                                                                           rdb::MemoryStore *memory            = nullptr);

  [[nodiscard]] rdb::Result<rdb::payload> constructAgsePayload(int length,                   //  _@(_,length)
                                                               int step,                     //  _@(step,_)
                                                               const std::string &instance,  //  instance@(_,_)
                                                               int windowIndex,              //  indeks LOGICZNY okna
                                                               int sourceIndexBase = 0) const;
  /*
   * This function will create aggregate payload based on the command and instance
   */
  [[nodiscard]] rdb::Result<rdb::payload> reduceFieldsToPayload(command_id cmd, const std::string &instance) const;

  /// Ta sama redukcja, zapisana wprost do pola 0 payloadu `target`. Cel musi miec dokladnie jedno pole
  /// o typie i szerokosci z reductionResultField() - tak buduje go query::descriptorFrom() dla reduktora.
  /// Goraca petla pisze tak do payloadu wejsciowego wezla, bez budowy wyniku i jego kopii w kazdym takcie.
  [[nodiscard]] rdb::Result<> reduceFieldsInto(command_id cmd, rdb::payload &target) const;

  /// Redukcja okna REKORDOWEGO nad polem TEGO strumienia - jedno przejście, cztery agregaty.
  ///
  /// Okno obejmuje rekordy logiczne `lastLogicalIndex-(group.width-1) ... lastLogicalIndex`.
  /// Który rekord jest ostatni, rozstrzyga wołający (dataModel::computeWindowAggregates);
  /// dla slotu n konsumenta jest to rekord n - patrz windowGroup. Z każdego rekordu wchodzi
  /// do redukcji JEDNA wartość - slot `group.slot` - więc okno redukuje dokładnie `width`
  /// wartości, po jednej na rekord.
  ///
  /// Rekordy spoza historii są pomijane (nie zerowane). Przy poprawnym planie ten przypadek
  /// nie występuje - origin gwarantuje, że całe okno leży w istniejącej części strumienia,
  /// a compiler::computeRequiredCapacities() zamawia dla niego pojemność.
  [[nodiscard]] rdb::Result<windowStats> reduceRecordWindow(const windowGroup &group, int lastLogicalIndex,
                                                            int sourceIndexBase) const;

  /// Wyniki okien tego taktu, indeksowane numerem grupy (query::windowGroups).
  ///
  /// Wypełnia dataModel::computeWindowAggregates() tuż przed constructOutputPayload(),
  /// czyta expressionEvaluator.
  std::vector<windowStats> windowValues;

  /*
   * This function will create OutputPayload based on all field from query
   * constructOutputPayload uses only data from inputPayload
   * inputPayload need to be filled first before this constructOutputPayload will be called.
   */
  [[nodiscard]] rdb::Result<> constructOutputPayload(const std::list<field> &fields) const;

  /*
   * This function will process all rules from query
   * constructRules uses data from outputPayload
   * outputPayload need to be filled first before this constructRules will be called
   */
  [[nodiscard]] rdb::Result<> constructRulesAndUpdate(const query &qry);

 private:
  streamInstance(qTree &coreInstance, query &qry, const std::string &storagePathParam, rdb::MemoryStore *memory);
  dumpManager dumpMgr;

  // Ksztalt deskryptora okna @(step,N) zalezy tylko od (ten strumien, |N|) i jest
  // niezmienny w trakcie dzialania (source->descriptor sie nie zmienia po
  // konstrukcji) -- cache unika odtwarzania go (string + alokacja na kazde pole
  // okna) przy kazdym wywolaniu constructAgsePayload. Wywolywane wylacznie z
  // watku przetwarzania (dataModel::processRows), nie z watku komunikacji IPC,
  // wiec bez potrzeby synchronizacji.
  mutable std::unordered_map<int, rdb::Descriptor> agseDescriptorCache_;

  // Najszerszy typ danych wyjscia i pole wyniku reduktora nad nim (reductionResultField). Deskryptor
  // wyjscia jest staly od create() - jak przy agseDescriptorCache_ - wiec reduceFieldsInto nie
  // przeglada go w kazdym takcie.
  std::pair<rdb::descFld, int> widestOutputField_{rdb::BYTE, 1};
  std::pair<rdb::descFld, int> reductionField_{rdb::BYTE, 1};
};
