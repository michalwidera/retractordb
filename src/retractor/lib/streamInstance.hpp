#pragma once

#include <memory>  // unique_ptr
#include <string>

#include "dumpManager.hpp"
#include "qTree.hpp"           // qTree
#include "rdb/storageacc.hpp"  // rdb::storage (transitively includes descriptor.hpp, payload.hpp)

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

  // This constructor will create data based on query
  explicit streamInstance(qTree &coreInstance, query &qry, const std::string &storagePathParam = "");

  rdb::payload constructAgsePayload(const int length,             //  _@(_,length)
                                    const int step,               //  _@(step,_)
                                    const std::string &instance,  //  instance@(_,_)
                                    const int storedRecordsCountDst);
  /*
   * This function will create aggregate payload based on the command and instance
   */
  rdb::payload reduceFieldsToPayload(command_id cmd, const std::string &instance);

  /*
   * This function will create OutputPayload based on all field from query
   * constructOutputPayload uses only data from inputPayload
   * inputPayload need to be filled first before this constructOutputPayload will be called.
   */
  void constructOutputPayload(const std::list<field> &fields);

  /*
   * This function will process all rules from query
   * constructRules uses data from outputPayload
   * outputPayload need to be filled first before this constructRules will be called
   */
  void constructRulesAndUpdate(const query &qry);

 private:
  dumpManager dumpMgr;
};
