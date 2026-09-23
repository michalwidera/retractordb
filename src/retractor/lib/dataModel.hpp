#pragma once

#include <map>
#include <memory>  // unique_ptr
#include <span>
#include <vector>

#include <boost/rational.hpp>

#include "streamInstance.hpp"  // streamInstance (transitively includes qTree.hpp, rdb/payload.hpp)
class dataModel {
 private:
  qTree &coreInstance_;

  std::map<std::string, std::string> directive_{{":STORAGE", ""}, {":SUBSTRAT", ""}, {":ROTATION", ""}};

  /// Instancja wykonawcza strumienia po nazwie, ktora MOZE nie istniec w modelu.
  ///
  /// Sciezka IPC (`xqry -d`, `xqry -t`) pyta o strumienie wypisane z planu, a plan i model
  /// potrafia sie rozjechac: getAdHoc() wnosi wezly do zywego drzewa przez importFrom(), po
  /// czym addQueryToModel() moze zawiesc, a wycofania nie ma. `qSet[id]` na takiej nazwie nie
  /// zglaszalo bledu, tylko WSTAWIALO pusty unique_ptr i zaraz go luskalo -- czyli SIGSEGV
  /// w odpowiedzi na komende. Brak nazwy jest tu bledem zgloszonym tak samo jak w
  /// qTree::getQuery: wyjatkiem, ktory handler zamienia w `error.response` dla klienta.
  [[nodiscard]] streamInstance &streamRuntime(const std::string &instance);

  [[nodiscard]] bool forwardRecordAvailable(const std::string &instance, int forwardIndex) const;
  [[nodiscard]] bool queryInputsAvailable(const query &qry, int logicalIndex);
  void bootstrapDeclaration(const query &qry);

 public:
  std::map<std::string, std::unique_ptr<streamInstance>> qSet;

  explicit dataModel(qTree &coreInstance);
  ~dataModel();

  dataModel() = delete;

  bool addQueryToModel(const std::string &id);
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
  rdb::payload fetchBack(const std::string &instance, int revOffset);

  /*
   * This function creates Input payload for ConstructOutputPayload data source
   * function need to be here because it access different streams from qSet
   */
  void constructInputPayload(const std::string &instance);

  /*
   * Wylicza okna rekordowe (MIN(pole:W:H) i rodzeństwo) dla jednego taktu strumienia.
   * Musi stać TU, a nie w streamInstance ani w ewaluatorze: okno czyta historię ŹRÓDŁA,
   * a dostęp do innych strumieni ma wyłącznie dataModel (qSet).
   */
  void computeWindowAggregates(const query &qry);

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

  /// @brief Nazwa pierwszego strumienia deklarowanego, którego źródło wyczerpało wejście.
  ///
  /// Pusty napis, gdy każde źródło ma jeszcze dane. Warunek stopu trybu --until-eof: przebieg
  /// trwa dopóty, dopóki KAŻDE wejście ma dane, bo od pierwszego wyczerpania rekordy liczyłyby
  /// się z all-null wstawionego za koniec pliku, a nie z danych.
  [[nodiscard]] std::string exhaustedInputStream() const;
};
