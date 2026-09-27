#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "appConfig.hpp"  // appcfg::kDefaultHistoryMemoryMib
#include "qTree.hpp"      // for qTree, query, token

/// Zatrzymuje kompilację, jeżeli którykolwiek węzeł planu nie ma wyliczonej wielkości.
///
/// Wspólna bramka obu przebiegów rachunku indeksu logicznego
/// (compiler::computeLogicalOrigin i compiler::computeStartupLatency). Uzasadnienie -
/// dlaczego nierozwiązany węzeł jest błędem, a nie stanem dopuszczalnym - jest przy
/// definicji w compiler.cpp.
///
/// Zadeklarowana w nagłówku, bo poza dwoma miejscami użycia w kompilatorze woła ją
/// bramka jednostkowa: reguły „plan bez nierozwiązanych węzłów" nie da się złamać
/// zapytaniem RQL (gramatyka na to nie pozwala), więc test musi podać mapę wprost.
void requireResolvedForEveryNode(const qTree &plan, const std::map<std::string, int> &resolved, std::string_view pass,
                                 std::string_view quantity);

struct compiler {
  explicit compiler(qTree &coreInstance) : coreInstance(coreInstance) {};
  compiler() = delete;

  std::string compile();
  std::vector<std::string> importFrom(qTree &source);

  /// Kasuje stan zebrany podczas kompilacji, zostawiajac wiazanie z tym samym drzewem.
  ///
  /// Potrzebne przy przeladowaniu planu w locie (`xqry --reset`): referencja `coreInstance`
  /// jest nierebindowalna, wiec plan wymienia sie PRZEZ ZAWARTOSC tego samego obiektu.
  /// Bez wyczyszczenia zapamietanych odwolan kompilacja nowego planu widzialaby strumienie
  /// poprzedniego.
  void reset();

  /// Budzet pamieci historii planu w MiB, `[limits] history_memory_mib` (A2 M11).
  ///
  /// Setter, a nie argument konstruktora, bo kompilator launchera powstaje, zanim wczyta sie
  /// konfiguracja. Wartosc domyslna jest wartoscia domyslna KLUCZA, a nie konfiguracja operatora,
  /// wiec kazde miejsce kompilujace plan w procesie serwera musi ja ustawic: start i `-c`
  /// (launcher), getAdHoc, attachAdHocRule i validatePlanText.
  void setHistoryMemoryBudget(int mib) { historyMemoryMib_ = mib; }

  /// Retencja strumieni plikowych bez RETENTION, `[storage] default_retention` (D8). Pusta = brak
  /// retencji. Ustawiana w tych samych miejscach co setHistoryMemoryBudget - inaczej .desc tego
  /// samego strumienia zalezalby od kanalu, ktorym przyszedl plan.
  void setDefaultRetention(rdb::retention_t retention) { defaultRetention_ = retention; }

  /// Strumienie skompilowanego planu rosnace na dysku bez granicy: nazwa i powod (D8).
  [[nodiscard]] std::vector<std::pair<std::string, std::string>> unboundedDiskStreams() const;

 private:
  qTree &coreInstance;
  int historyMemoryMib_       = appcfg::kDefaultHistoryMemoryMib;
  bool restrictSelectSharing_ = false;
  std::set<std::string> selectSharingScope_;
  rdb::retention_t defaultRetention_{.segments = 0, .capacity = 0};
  /// Nazwy strumieni, po których sięgnął UŻYTKOWNIK, per zapytanie - sprawdzane przez bramkę
  /// przeplotu w localizeFieldOffsets(). Zbierane z dwóch miejsc, bo formy zapisu różnią się
  /// momentem, w którym znana jest nazwa strumienia:
  ///  * `A[0]` i `A.pole` - snapshotNamedSourceRefs(), przed pierwszym przebiegiem, bo później
  ///    buildOutputSchema() syntetyzuje własne PUSH_ID2 i typ tokenu przestaje odróżniać
  ///    użytkownika od kompilatora (te syntetyczne dwuznaczne nie są);
  ///  * goła nazwa pola - resolveTokenReferences(), bo nazwa strumienia powstaje dopiero
  ///    z wyszukania pola w schematach argumentów. PUSH_ID3 wystawia wyłącznie parser.
  std::map<std::string, std::set<std::string>> namedSourceRefs_;
  /// Szerokosc rekordu FROM i rozpietosci nazw w nim, zapamietane na czas resolveFieldReferences().
  /// Przebieg przepisuje wylacznie tokeny programow pol, wiec schematy i programy FROM sie w nim nie
  /// zmieniaja. Bez pamieci descriptorFrom() budowal sie od nowa dla kazdego odwolania, czyli
  /// kwadratowo wzgledem szerokosci wezla: `SELECT * FROM src@(1,65536)` kompilowal sie ok. 2 minut.
  std::map<std::string, int> fromWidthMemo_;
  std::map<std::pair<std::string, std::string>, std::optional<int>> fromSpanMemo_;
  [[nodiscard]] std::string checkRecordShape(const query &q, std::int64_t *planElements) const;
  std::string checkInputRecord(query &q);
  std::list<field> buildOutputSchema(const std::string &sName1, const std::string &sName2, token &cmd_token);
  [[nodiscard]] std::optional<rdb::rField> sourceFieldAt(const std::string &streamId, int flatIndex) const;
  std::string composeStreamName(const std::string &sName1, const std::string &sName2, const token &cmd);
  std::string resolveTokenReferences(std::list<token> &lProgram, query &q, const std::string &ruleName);
  void snapshotNamedSourceRefs();

  // compile chain steps
  std::string checkFunctionCalls();
  std::string checkStreamReducerFieldRefs();
  std::string expandStreamGenerators();
  std::string substituteOrdinal(query &instance, int ordinal);
  std::string resolveStreamIntervals();
  std::string extractIntermediateStreams();
  std::string expandSchemaWildcards();
  std::string expandIndexWildcards(query &q);
  std::optional<int> sourceSpanInFrom(query &q, const std::string &name);
  std::optional<int> sourceSpanIn(query &node, int nodeWidth, const std::string &name);
  std::optional<int> descendSpan(const std::string &nodeId, int width, const std::string &name);
  std::string resolveFieldReferences();
  std::string resolveWindowAggregates();
  std::string inferFieldShapes();
  std::string checkRuleConditionShapes();
  std::string localizeFieldOffsets();
  std::map<std::string, int> sourceOffsetsInFrom(query &q, std::set<std::string> &viaInterleave);
  void collectTransitiveOffsets(const std::string &srcId, int baseOffset, bool viaHash, std::map<std::string, int> &result,
                                std::set<std::string> &viaInterleave);
  std::string validateSubstratNameUniqueness();
  std::string validateConstraints();
  std::map<std::string, int> computeRequiredCapacities();
  std::string applyCapacitiesToStreams(const std::map<std::string, int> &capMap);
  std::string checkHistoryMemory();
  std::string applyDiskRetention();
  [[nodiscard]] std::map<std::string, std::vector<std::string>> snapshotUserFieldNames() const;
  [[nodiscard]] std::string verifyUserFieldNamesPreserved(const std::map<std::string, std::vector<std::string>> &before) const;
  std::string computeLogicalOrigin();
  std::string computeStartupLatency();
  std::string factorMatchedHashTimeMoves();
  std::string simplifyFieldExpressions();
  void retargetSchemaReferences(query &q, const std::string &oldName, const std::string &newName);
  void replaceStreamReferences(const std::string &oldName, const std::string &newName);
  std::string deduplicateSubstrats();
  std::string shareEquivalentSelectComputations();
};
