#pragma once

#include <memory>  // unique_ptr
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "dumpManager.hpp"
#include "qTree.hpp"        // qTree
#include "rdb/storage.hpp"  // rdb::storage (transitively includes descriptor.hpp, payload.hpp)

/// @brief Klasa streamInstance umożliwia tworzenie zawartości poszczególnych rekordów
///
/// Obiekt klasy streamInstance powinien:
/// - być inicjalizowany z instancją qTree i query, na podstawie których będzie tworzył zawartość rekordów (payload).
/// - w trakcie inicjalizacji powinna być możliwość dodania dodatkowego parametru, np. ścieżki do katalogu, w którym będą przechowywane dane (storagePathParam).
/// - tworzyć payloady na podstawie zapytań, w tym agregacje i dozwolone operacje na polach.
/// - zarządzać regułami aktualizacji danych zgodnie z definicjami w query.
/// - współpracować z klasą dumpManager do rejestrowania i wykonywania zadań dumpowania danych (akcja DUMP); wywołania systemowe (akcja SYSTEM) są wykonywane bezpośrednio.
/// - łączyć dane wejściowe (inputPayload) z logiką zapytania, aby generować dane wyjściowe (outputPayload) gotowe do zapisania w magazynie.

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
  /// te dwie wielkości — outputPayload->getRecordsCount() pozostaje indeksem elementu, ten licznik jest
  /// pozycją na siatce slotów.
  size_t elapsedSlots = 0;

  /// @brief Indeks logiczny fizycznego rekordu 0 tej instancji.
  ///
  /// Plan uruchamiany od początku używa query::logicalOrigin. Instancja dodana ad hoc
  /// dostaje bazę dopiero w pierwszym należnym jej slocie bieżącej osi czasu; do tego
  /// momentu std::nullopt odróżnia ją od strumienia startowego oczekującego na origin/ogon.
  std::optional<int> logicalIndexBase;

  // This constructor will create data based on query
  explicit streamInstance(qTree &coreInstance, query &qry, const std::string &storagePathParam = "");

  [[nodiscard]] rdb::payload constructAgsePayload(int length,                   //  _@(_,length)
                                                  int step,                     //  _@(step,_)
                                                  const std::string &instance,  //  instance@(_,_)
                                                  int windowIndex,              //  indeks LOGICZNY okna
                                                  int sourceIndexBase = 0) const;
  /*
   * This function will create aggregate payload based on the command and instance
   */
  [[nodiscard]] rdb::payload reduceFieldsToPayload(command_id cmd, const std::string &instance) const;

  /// Redukcja okna REKORDOWEGO nad polem TEGO strumienia — jedno przejście, cztery agregaty.
  ///
  /// Okno obejmuje rekordy logiczne `lastLogicalIndex-(group.width-1) ... lastLogicalIndex`.
  /// Który rekord jest ostatni, rozstrzyga wołający (dataModel::computeWindowAggregates);
  /// dla slotu n konsumenta jest to `(n+1)*krok-1` — patrz windowGroup. Z każdego
  /// rekordu wchodzą do redukcji wszystkie sloty płaskie pola, więc `INTEGER[24]` przy
  /// szerokości 10 daje 240 wartości.
  ///
  /// Rekordy spoza historii są pomijane (nie zerowane). Przy poprawnym planie ten przypadek
  /// nie występuje — origin gwarantuje, że całe okno leży w istniejącej części strumienia,
  /// a compiler::computeRequiredCapacities() zamawia dla niego pojemność.
  [[nodiscard]] windowStats reduceRecordWindow(const windowGroup &group, int lastLogicalIndex, int sourceIndexBase) const;

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
  void constructOutputPayload(const std::list<field> &fields) const;

  /*
   * This function will process all rules from query
   * constructRules uses data from outputPayload
   * outputPayload need to be filled first before this constructRules will be called
   */
  void constructRulesAndUpdate(const query &qry);

 private:
  dumpManager dumpMgr;

  // Ksztalt deskryptora okna @(step,N) zalezy tylko od (ten strumien, |N|) i jest
  // niezmienny w trakcie dzialania (source->descriptor sie nie zmienia po
  // konstrukcji) -- cache unika odtwarzania go (string + alokacja na kazde pole
  // okna) przy kazdym wywolaniu constructAgsePayload. Wywolywane wylacznie z
  // watku przetwarzania (dataModel::processRows), nie z watku komunikacji IPC,
  // wiec bez potrzeby synchronizacji.
  mutable std::unordered_map<int, rdb::Descriptor> agseDescriptorCache_;
};
