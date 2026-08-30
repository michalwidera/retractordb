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

/// Grupa okna rekordowego — cel jednego przejscia po historii zrodla.
///
/// Okno rekordu wyjsciowego n obejmuje rekordy zrodla `(n+1)*step-width ... (n+1)*step-1`,
/// czyli konczy sie na OSTATNIM rekordzie zrodla nalezacym do slotu n. Stad interwal wyjscia
/// `step * interwal zrodla`, a brzeg:
///   origin = ceil((origin zrodla + width) / step) - 1
///   ogon   = ceil(ogon zrodla / step)
///
/// Konwencja jest wymuszona, nie wybrana. Stemplowanie koncem na `n*step` — czyli wprost
/// przeniesione z AGSE — daje przy width=step=10 okna [1..10], [11..20], ...: rekord 0 nie
/// trafia do ZADNEGO okna. Kafelkowanie ma zaczynac sie od rekordu 0, bo strumien zaczyna
/// sie od rekordu 0. Konwencja powyzej daje [0..9], [10..19], ... i zachowuje przy okazji
/// dwie wlasnosci: `AGG(pole:1)` jest tozsame z reduktorem `FROM AGG(strumien)` (okno n to
/// rekord n), a okno siega po NAJSWIEZSZY rekord dostepny w slocie n.
///
/// Wzgledem bazowego UC07 znika przy tym kompensacja `>1`: tam agregat wydany pod indeksem
/// n opisuje sekunde n-1 (patrz uc07/README.md), tutaj opisuje sekunde n.
///
/// `firstSlot` i `slotCount` opisuja pole w PLASKIM ukladzie rekordu zrodla: `INTEGER[24]`
/// to jeden wpis deskryptora, ale 24 sloty, i wszystkie 24 wchodza do okna. Redukcja obejmuje
/// wiec `width * slotCount` wartosci.
struct windowGroup {
  std::string source;  ///< strumien zrodlowy (nazwa fizyczna po ekspansji generatorow)
  int firstSlot = 0;   ///< pierwszy slot plaski pola w rekordzie zrodla
  int slotCount = 1;   ///< liczba slotow plaskich pola (1 dla skalara, N dla `T[N]`)
  int width     = 0;   ///< szerokosc okna w REKORDACH
  int step      = 1;   ///< krok w REKORDACH

  bool sameAs(const windowGroup &other) const {
    return source == other.source && firstSlot == other.firstSlot && slotCount == other.slotCount && width == other.width &&
           step == other.step;
  }
};

/// Wynik JEDNEGO przejścia po oknie grupy — wszystkie cztery agregaty naraz.
///
/// Liczymy komplet, a nie tylko żądany agregat, bo przejście po oknie jest kosztem
/// dominującym, a same porównania są darmowe. Dzięki temu `MIN(cells:10:10)` i
/// `MAX(cells:10:10)` w jednej liście SELECT czytają historię źródła raz.
///
/// `count` liczy wartości NIE-NULL. NULL-e są pomijane (nie zerowane), a okno bez ani
/// jednej wartości daje NULL we wszystkich czterech polach — zasada brzegu: brak wyniku
/// nie jest zerem.
struct windowStats {
  rdb::descFldVT minValue{std::monostate{}};
  rdb::descFldVT maxValue{std::monostate{}};
  rdb::descFldVT sumValue{std::monostate{}};
  rdb::descFldVT avgValue{std::monostate{}};
  int count = 0;
};

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

  /// Grupy okien rekordowych tego zapytania — jedna pozycja na czworke
  /// (zrodlo, pole, szerokosc, krok). Token WINDOW_* niesie indeks do tej tabeli.
  ///
  /// Tabela jest skladowa ZAPYTANIA, nie planu, i to jest wymog poprawnosci. Zapytania
  /// wedruja miedzy drzewami: compiler::importFrom() przenosi zapytanie ad hoc do zywego
  /// planu, a expandStreamGenerators() kopiuje szablon na N instancji. Indeks globalny
  /// przezylby te wedrowke tylko przy przenumerowaniu tabeli w kazdym z tych miejsc;
  /// tabela lokalna jedzie razem z zapytaniem i pozostaje spojna sama z siebie.
  ///
  /// Grupa niesie WSZYSTKIE cztery wartosci (min, max, sum, count) z jednego przejscia po
  /// oknie, wiec `MIN(cells:10:10)` i `MAX(cells:10:10)` w tej samej liscie SELECT czytaja
  /// okno raz — o to chodzi we wspolnym stanie dla agregatow o identycznym ksztalcie.
  std::vector<windowGroup> windowGroups;

  [[nodiscard]] bool hasWindowAggregates() const { return !windowGroups.empty(); }

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
