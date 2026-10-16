#pragma once

#include <cstdint>
#include <expected>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "appConfig.hpp"  // appcfg::kDefaultHistoryMemoryMib
#include "qTree.hpp"      // for qTree, query, token

/// Funkcja requireResolvedForEveryNode powinna:
/// - zwracać "OK", gdy mapa resolved zawiera identyfikator każdego węzła planu, także deklaracji i dyrektyw.
///   Test: `xcompiler.complete_resolution_passes_the_gate`, `xcompiler.resolution_gate_includes_directives_and_names_the_first_missing_node_and_pass`.
/// - dla pierwszego węzła spoza mapy zwracać przez internalCompilerError status z nazwą przebiegu pass, wielkości quantity i identyfikatorem węzła.
///   Test: `xcompiler.unresolved_node_is_a_compilation_error`, `xcompiler.resolution_gate_includes_directives_and_names_the_first_missing_node_and_pass`.
///
/// @note Wspolna bramka przebiegow compiler::computeLogicalOrigin i compiler::computeStartupLatency. Do
///       2026-08-07 wezel pominiety dostawal po cichu domyslne 0, czyli rezim ZANIZAJACY ogon. Legalny plan
///       nie zostawia wezla nierozwiazanego, wiec to defekt kompilatora; regule da sie zlamac tylko z testu
///       jednostkowego, ktory podaje mape wprost - stad deklaracja w naglowku.
[[nodiscard]] std::string requireResolvedForEveryNode(const qTree &plan, const std::map<std::string, int> &resolved,
                                                      std::string_view pass, std::string_view quantity);

/// Przedrostek statusu compile() dla zlamanego niezmiennika kompilatora - bledu w kodzie, a nie
/// w planie. Wolajacy (Engine::compile, demon) odrozniaja go od bledu planu uzytkownika.
inline constexpr std::string_view kInternalCompilerError = "internal compiler error: ";

/// Funkcja internalCompilerError powinna:
/// - zwracać message poprzedzony przedrostkiem kInternalCompilerError.
///   Test: `PlanSource.internal_compiler_status_is_prefixed_logged_and_recognized`.
/// - zapisywać ten sam tekst w logu na poziomie ERROR.
///   Test: `PlanSource.internal_compiler_status_is_prefixed_logged_and_recognized`.
///
/// @note Zlamany niezmiennik kompilatora nie moze wrocic do klienta jako blad jego planu: Engine::compile
///       zglasza go jako Errc::Logic, a demon traktuje jak blad krytyczny.
[[nodiscard]] std::string internalCompilerError(const std::string &message);

/// Funkcja isInternalCompilerError powinna:
/// - zwracać true wyłącznie dla statusu zaczynającego się od kInternalCompilerError, czyli błędu wewnętrznego kompilatora, a nie błędu planu.
///   Test: `PlanSource.internal_compiler_status_is_prefixed_logged_and_recognized`.
[[nodiscard]] bool isInternalCompilerError(std::string_view status);

/// Obiekt klasy compiler powinien:
/// - przez compile zwracać OK albo status błędu planu, a złamany niezmiennik kompilatora zgłaszać statusem z przedrostkiem kInternalCompilerError, nigdy wyjątkiem.
///   Test: `xparser.check_compile`, `xcompiler.still_reports_true_circular_dependency`, `xcompiler.malformed_intermediate_operator_is_rejected_before_iterator_underflow`.
/// - przed rozwinięciem generatorów odrzucać nieznane funkcje, niepoprawną arność i odwołania pól w reduktorach strumieni.
///   Test: `xparser.unknown_function_is_rejected_at_compile_time`, `xparser.declared_width_is_rejected_for_functions_without_width`, `xcompiler.rejects_a_stream_reducer_used_as_a_field_reference`.
/// - podczas rozwijania odrzucać niepoprawne generatory, a po ich rozwinięciu, przed wydzieleniem substratów, odrzucać
///   odwołania FROM do strumieni spoza planu i okna AGSE o niedodatnim kroku.
///   Test: `xcompiler.rejects_zero_sized_generator`, `xcompiler.rejects_generator_without_ordinal`, `xcompiler.refuses_non_positive_operator_intervals_without_parser`.
///   Test: `xcompiler.missing_from_source_and_non_positive_agse_step_are_refused_after_generator_expansion`.
/// - rozwijać generatory strumieni do N instancji i odrzucać plan, który po rozwinięciu przekracza limit liczby strumieni.
///   Test: `xcompiler.generator_plan_is_identical_to_hand_written_plan`, `xcompiler.generator_substitutes_ordinal_as_a_value`, `xcompiler.stream_count_after_generator_expansion_is_limited`.
/// - sprowadzać każdą klauzulę FROM do jednego operatora strumieniowego przez wydzielenie strumieni pośrednich (substratów) o nazwach mieszczących się w budżecie długości, i odrzucać różne programy pod tą samą nazwą substratu.
///   Test: `xcompiler.stream_function_matches_the_two_step_form`, `xcompiler.wide_from_clause_falls_back_to_digest`, `xcompiler.substrate_names_stay_identifiers`.
///   Test: `xcompiler.refuses_colliding_substrate_programs_before_topological_sort`.
/// - rozwijać wieloznaczniki schematu, rozwiązywać interwały strumieni, odwołania pól i okna rekordowe, wyznaczać typy pól i odrzucać warunki reguł o niewłaściwym kształcie.
///   Test: `xcompiler.index_wildcard_expands_in_place_beside_other_select_items`, `xcompiler.resolves_intervals_independently_of_plan_order`, `xcompiler.window_group_addresses_a_single_flat_slot`.
///   Test: `xcompiler.requested16_three_documented_boundaries`, `xcompiler.rule_condition_reaching_another_stream_is_refused`.
/// - wyliczać dla każdego węzła origin i ogon dokładną arytmetyką wymierną, odrzucając plan z węzłem bez wyliczonej wartości.
///   Test: `xcompiler.computes_startup_latency`, `xcompiler.exact_tail_for_subtract_and_dehash`, `xcompiler.every_node_of_a_nine_class_plan_is_resolved`.
///   Test: `xcompiler.unresolved_node_is_a_compilation_error`, `xcompiler.agse_logical_origin_beyond_int_is_a_plan_error`.
/// - wyznaczać pojemność historii każdego strumienia wymaganą przez konsumentów i odrzucać plan przekraczający budżet
///   pamięci historii ustawiony przez setHistoryMemoryBudget; domyślną retencję dyskową z setDefaultRetention stosować
///   do niedeklarowanych strumieni w magazynach DEFAULT i DIRECT bez własnego RETENTION.
///   Test: `capacities.add_declared_matches_event_table`, `capacities.subtract_declared_matches_event_table`, `xcompiler.agse_capacity_period_is_computed_without_int_overflow`.
///   Test: `xcompiler.stream_history_in_memory_is_limited_by_the_config_budget`, `xcompiler.default_retention_bounds_disk_streams_without_retention`, `xcompiler.default_retention_too_small_for_the_plan_is_a_compilation_error`.
/// - zwracać plan w porządku topologicznym, w którym każdy producent poprzedza swoich konsumentów, niezależnie od tego, które optymalizacje zadziałały.
///   Test: `xcompiler.compiled_plan_places_every_producer_before_its_consumers`.
/// - stosować optymalizacje faktoryzacji przesunięć przez przeplot, deduplikacji substratów, upraszczania wyrażeń i współdzielenia równoważnych SELECT wyłącznie za odpowiednim przełącznikiem RDB_OPT_*, tak aby wyłączenie każdego z nich nie zmieniało wartości wyników, a ogon co najwyżej wydłużało.
///   Test: `xcompiler.toggles_substrate_deduplication`, `xcompiler.toggles_matched_hash_time_move_factorization`, `xcompiler.shares_commutative_add_select_computation`.
///   Test: `xcompiler.window_subexpression_goes_through_expression_simplification`, `ProofOracle.causal_shift_matching_read`, `ProofOracle.exact_tail_rewrite`.
/// - po każdej optymalizacji sprawdzać, że nazwy pól widoczne dla użytkownika nie zmieniły się, i odrzucać plan, gdy zmieniły się.
///   Test: `xcompiler.rewrites_preserve_observable_field_names`.
///   Kontrola strukturalna: compiler.cpp - snapshot i verifyUserFieldNamesPreserved po kazdym z czterech przebiegow; odmowa zmiany nazw ma jawny status. Test sprawdza zachowanie nazw, nie wymusza odmowy.
/// - przez importFrom dopisywać do planu węzły źródła nieobecne w nim, z pominięciem dyrektyw, zwracać ich nazwy i ograniczać współdzielenie SELECT w następnej kompilacji do tych węzłów, tak aby nie przepisać istniejących instancji strumieni.
///   Test: `xcompiler.does_not_rewrite_existing_queries_during_import`, `xcompiler.import_returns_only_new_non_directive_nodes_and_reset_forgets_previous_plan`.
/// - przez reset kasować stan zebrany podczas kompilacji, zachowując powiązanie z tym samym planem, tak aby kompilacja nowego planu nie widziała strumieni poprzedniego.
///   Test: `xcompiler.import_returns_only_new_non_directive_nodes_and_reset_forgets_previous_plan`.
/// - przez unboundedDiskStreams zwracać strumienie wynikowe zapisywane na dysku bez granicy retencji wraz z powodem.
///   Test: `xcompiler.default_retention_bounds_disk_streams_without_retention`.
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
  /// "OK" i schemat w `schema` albo status odmowy (schemat nietkniety). Status, a nie
  /// std::expected<std::list<field>, std::string>: GCC 14 z -O2 zglaszal na tej postaci falszywe
  /// -Wfree-nonheap-object (napis w unii expected po inliningu).
  [[nodiscard]] std::string buildOutputSchema(const std::string &sName1, const std::string &sName2, token &cmd_token,
                                              std::list<field> &schema);
  [[nodiscard]] std::optional<rdb::rField> sourceFieldAt(const std::string &streamId, int flatIndex) const;
  std::string composeStreamName(const std::string &sName1, const std::string &sName2, const token &cmd);
  std::string resolveTokenReferences(std::list<token> &lProgram, query &q, const std::string &ruleName);
  void snapshotNamedSourceRefs();

  // compile chain steps
  std::string checkFunctionCalls();
  std::string checkStreamReducerFieldRefs();
  std::string expandStreamGenerators();

  /// Kazde odwolanie do strumienia w klauzuli FROM wskazuje strumien planu, a okno `@` ma krok
  /// dodatni. Pierwszy przebieg po rozwinieciu generatorow: kazdy pozniejszy siega po wezly przez
  /// qTree::getQuery() i query::descriptorFrom(), ktore traktuja oba warunki jako niezmienniki.
  std::string checkStreamReferences();
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
  std::expected<std::map<std::string, int>, std::string> computeRequiredCapacities();
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
