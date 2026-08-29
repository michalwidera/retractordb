#pragma once

#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "qTree.hpp"  // for qTree, query, token

/// Zatrzymuje kompilację, jeżeli którykolwiek węzeł planu nie ma wyliczonej wielkości.
///
/// Wspólna bramka obu przebiegów rachunku indeksu logicznego
/// (compiler::computeLogicalOrigin i compiler::computeStartupLatency). Uzasadnienie —
/// dlaczego nierozwiązany węzeł jest błędem, a nie stanem dopuszczalnym — jest przy
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

  /// Rodziny rozwiniete przez expandStreamGenerators(): nazwa szablonu -> nazwy instancji.
  ///
  /// Potrzebne POZA kompilatorem, bo generator lamie zalozenie „jedna linia RQL = jeden
  /// strumien”, na ktorym opiera sie sprzatanie nieaktualnych artefaktow w launcherze.
  /// Mapa jest jedynym zrodlem tej wiedzy: rozpoznawanie instancji po ksztalcie nazwy
  /// (`szablon$n`) myliloby sie z recznie zadeklarowanym strumieniem o takiej nazwie,
  /// a stawka jest kasowanie plikow.
  [[nodiscard]] const std::map<std::string, std::vector<std::string>> &generatedStreams() const { return generatedStreams_; }

 private:
  qTree &coreInstance;
  bool restrictSelectSharing_ = false;
  std::set<std::string> selectSharingScope_;
  /// Nazwy strumieni, po których sięgnął UŻYTKOWNIK, per zapytanie — sprawdzane przez bramkę
  /// przeplotu w localizeFieldOffsets(). Zbierane z dwóch miejsc, bo formy zapisu różnią się
  /// momentem, w którym znana jest nazwa strumienia:
  ///  * `A[0]` i `A.pole` — snapshotNamedSourceRefs(), przed pierwszym przebiegiem, bo później
  ///    buildOutputSchema() syntetyzuje własne PUSH_ID2 i typ tokenu przestaje odróżniać
  ///    użytkownika od kompilatora (te syntetyczne dwuznaczne nie są);
  ///  * goła nazwa pola — resolveTokenReferences(), bo nazwa strumienia powstaje dopiero
  ///    z wyszukania pola w schematach argumentów. PUSH_ID3 wystawia wyłącznie parser.
  std::map<std::string, std::set<std::string>> namedSourceRefs_;
  std::map<std::string, std::vector<std::string>> generatedStreams_;
  std::list<field> buildOutputSchema(const std::string &sName1, const std::string &sName2, token &cmd_token);
  std::string composeStreamName(const std::string &sName1, const std::string &sName2, const token &cmd);
  void resolveTokenReferences(std::list<token> &lProgram, query &q);
  void snapshotNamedSourceRefs();

  // compile chain steps
  std::string expandStreamGenerators();
  std::string substituteOrdinal(query &instance, int ordinal);
  std::string validateGeneratedFieldIndex(const std::string &owner, const std::string &source, int index);
  std::string resolveStreamIntervals();
  std::string extractIntermediateStreams();
  std::string expandSchemaWildcards();
  std::string expandIndexWildcards();
  std::string resolveFieldReferences();
  std::string localizeFieldOffsets();
  void collectTransitiveOffsets(const std::string &srcId, int baseOffset, bool viaHash, std::map<std::string, int> &result,
                                std::set<std::string> &viaInterleave);
  std::string validateSubstratNameUniqueness();
  std::string validateConstraints();
  std::map<std::string, int> computeRequiredCapacities();
  std::string applyCapacitiesToStreams(const std::map<std::string, int> &capMap);
  std::map<std::string, std::vector<std::string>> snapshotUserFieldNames() const;
  std::string verifyUserFieldNamesPreserved(const std::map<std::string, std::vector<std::string>> &before) const;
  std::string computeLogicalOrigin();
  std::string computeStartupLatency();
  std::string factorMatchedHashTimeMoves();
  std::string simplifyFieldExpressions();
  void retargetSchemaReferences(query &q, const std::string &oldName, const std::string &newName);
  void replaceStreamReferences(const std::string &oldName, const std::string &newName);
  std::string deduplicateSubstrats();
  std::string shareEquivalentSelectComputations();
};
