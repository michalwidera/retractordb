#pragma once

#include <iostream>
#include <list>
#include <string>
#include <tuple>
#include <vector>

#include <boost/rational.hpp>

#include "field.hpp"
#include "rdb/descriptor.hpp"
#include "rdb/faccmemory.hpp"
#include "rule.hpp"
#include "token.hpp"

class qTree;

inline constexpr size_t kGeneratedPrefixLength = sizeof("STREAM_") - 1;

/// Typ i długość pola, w którym mieści się wynik redukcji (AVG/MIN/MAX/SUMC).
///
/// K24/D4: schemat wyjścia redukcji budowany przez kompilator jest zawsze
/// RATIONAL, a rachunek redukcji idzie po boost::rational<int>. Deskryptor
/// wejściowy (query::descriptorFrom) i payload wyniku
/// (streamInstance::reduceFieldsToPayload) używały typu ŹRÓDŁA, więc wartość
/// przechodziła przez rational_cast<int> — średnia z dwóch pól o sumie
/// nieparzystej dawała 2000003/2 jako 1000001/1. Reguła jest tu jedna dla obu
/// miejsc właśnie po to, żeby nie mogły się ponownie rozjechać.
inline std::pair<rdb::descFld, int> reductionResultField(rdb::descFld sourceType, int sourceLength) {
  const bool arithmetic =
      sourceType == rdb::BYTE || sourceType == rdb::INTEGER || sourceType == rdb::UINT || sourceType == rdb::RATIONAL;
  if (!arithmetic) return {sourceType, sourceLength};
  return {rdb::RATIONAL, static_cast<int>(sizeof(boost::rational<int>))};
}

class query {
  void fillDescriptor(const std::list<field> &lSchemaVar, rdb::Descriptor &val, const std::string &id);

 public:
  explicit query(boost::rational<int> rInterval, std::string id);
  query();

  std::list<std::string> getFieldNamesList();

  std::string id;
  std::string filename;
  boost::rational<int> rInterval = 0;

  /// @brief Licznosc rodziny generatora strumieni; 0 = zwyczajny strumien, nie generator.
  ///
  /// Ustawia parser z zapisu `STREAM cell[24]`, kasuje compiler::expandStreamGenerators()
  /// przy tworzeniu kazdej z N instancji. Zadne zapytanie z nieujemna wartoscia tego pola nie
  /// moze przezyc pierwszego przebiegu kompilacji — dalsze przebiegi go nie znaja.
  ///
  /// UJEMNA wartosc znaczy „to nie jest generator", a nie „generator o zerowej licznosci".
  /// Rozroznienie jest konieczne, bo `STREAM cell[0]` jest bledem, ktory trzeba zglosic —
  /// przy wartowniku 0 zapis ten stalby sie po cichu zwyklym strumieniem `cell`.
  static constexpr int notAGenerator = -1;
  int generatorSize                  = notAGenerator;

  /// @brief Ogon strumienia: liczba początkowych slotów WŁASNEGO interwału, w których wynik nie jest jeszcze zdefiniowany.
  ///
  /// Zasada brzegu strumienia: te sloty NIE są rekordami — ani zerami, ani NULL-ami. Strumień po prostu jeszcze nie
  /// emituje, a system raportuje ogon. NULL jest zarezerwowany dla wartości pochłaniającej: danych oczekiwanych
  /// a nieobecnych albo wyniku nieistniejącego w zbiorze wartości (np. dzielenie przez zero). NULL nigdy nie oznacza
  /// „system przewiduje tu miejsce na dane”.
  ///
  /// Wyliczane dokładną arytmetyką wymierną przez compiler::computeStartupLatency().
  int startupLatency = 0;

  /// @brief Początek logiczny strumienia: indeks PIERWSZEGO rekordu, który w ogóle istnieje.
  ///
  /// Rekordy o indeksie mniejszym od origin nie powstają — nie dlatego, że jeszcze ich nie ma (od tego jest ogon),
  /// tylko dlatego, że ich definicja sięga przed początek strumienia źródłowego. Okno `@(k,L)` stemplowane końcem
  /// przedziału obejmuje pozycje n*k-(|L|-1) ... n*k, więc dla małych n sięga w nieistniejącą przeszłość; te sloty
  /// są niedefiniowalne, a nie „puste”. Zasada brzegu z komentarza przy startupLatency zabrania wypełniać je NULL-em.
  ///
  /// Indeks logiczny jest walutą wszystkich odwzorowań między strumieniami (Add, Hash, Subtract, Theta, AGSE).
  /// Przeliczenie na fizyczny offset w buforze robi wyłącznie dataModel::fetchForward().
  ///
  /// Wyliczane przez compiler::computeLogicalOrigin().
  int logicalOrigin = 0;

  bool isDisposable = false;
  bool isOneShot    = false;
  bool isHold       = false;
  bool isSubstrat   = false;

  std::list<field> lSchema;
  std::list<token> lProgram;

  std::list<rule> lRules;

  rdb::retention_t retention            = rdb::retention_t{.segments = 0, .capacity = 0};  // Retention segments and capacity
  std::pair<std::string, size_t> policy = std::make_pair("DEFAULT", rdb::memoryFile::no_retention);
  std::string storage_policy            = "DEFAULT";

  [[nodiscard]] bool isDeclaration() const { return lProgram.empty(); }
  bool isReductionRequired();
  [[nodiscard]] bool isGenerated() const { return id.compare(0, kGeneratedPrefixLength, "STREAM_") == 0; }
  [[nodiscard]] bool isCompilerDirective() const { return !id.empty() && id[0] == ':'; }
  bool is(command_id command);

  std::vector<std::string> getDepStream();

  int getFieldIndex(const field &f);

  void reset();

  rdb::Descriptor descriptorStorage();
  rdb::Descriptor descriptorFrom(qTree &coreInstance);

  friend std::ostream &operator<<(std::ostream &os, const query &s);
};

bool operator<(const query &lhs, const query &rhs);

std::tuple<std::string, std::string, token> GetArgs(std::list<token> &prog);

bool isThere(const std::vector<query> &v, const std::string &query_name);
