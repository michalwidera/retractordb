#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <list>
#include <optional>
#include <string>
#include <string_view>
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

/// Funkcja reductionResultField powinna:
/// - [query.reduction-result-field.1~1] dla typu źródła BYTE, INTEGER, UINT albo RATIONAL zwracać pole RATIONAL o długości sizeof(boost::rational<int>).
/// - [query.reduction-result-field.2~1] dla każdego innego typu źródła zwracać typ i długość źródła bez zmian.
///
/// @note K24/D4: rachunek redukcji (AVG/MIN/MAX/SUMC) idzie po boost::rational<int>. Deskryptor wejsciowy
///       (query::descriptorFrom) i payload wyniku (streamInstance::reduceFieldsToPayload) uzywaly typu
///       ZRODLA, wiec wartosc przechodzila przez rational_cast<int> - srednia 2000003/2 dawala 1000001/1.
///       Regula jest tu jedna dla obu miejsc, zeby nie mogly sie ponownie rozjechac.
inline std::pair<rdb::descFld, int> reductionResultField(rdb::descFld sourceType, int sourceLength) {
  const bool arithmetic =
      sourceType == rdb::BYTE || sourceType == rdb::INTEGER || sourceType == rdb::UINT || sourceType == rdb::RATIONAL;
  if (!arithmetic) return {sourceType, sourceLength};
  return {rdb::RATIONAL, static_cast<int>(sizeof(boost::rational<int>))};
}

/// Grupa okna rekordowego - cel jednego przejscia po historii zrodla.
///
/// Okno rekordu wyjsciowego n obejmuje rekordy zrodla `n-(width-1) ... n`, czyli konczy sie
/// na rekordzie o TYM SAMYM indeksie logicznym. Interwal wyjscia jest rowny interwalowi
/// zrodla - lista SELECT nie rusza osi czasu - a brzeg:
///   origin = origin zrodla + width - 1
///   ogon   = ogon zrodla
///
/// Okno jest wiec zawsze PRZESUWNE. Kroku ta konstrukcja nie ma: okno co k rekordow zapisuje
/// sie po stronie FROM, bo tam mieszka kazda zmiana taktu (patrz regula `window_agg`
/// w RQL.g4). Stemplowanie koncem daje przy okazji tozsamosc `AGG(pole:1)` z reduktorem
/// `FROM AGG(strumien)` - okno n to rekord n - i gwarantuje, ze okno siega po NAJSWIEZSZY
/// rekord dostepny w slocie n.
///
/// Wzgledem bazowego UC07 znika przy tym kompensacja `>1`: tam agregat wydany pod indeksem
/// n opisuje sekunde n-1 (patrz uc07/README.md), tutaj opisuje sekunde n.
///
/// Okno redukuje DOKLADNIE `width` wartosci: po jednej z kazdego z `width` kolejnych rekordow.
/// Redukcja idzie wiec wylacznie po CZASIE.
///
/// Do 2026-08-31 argumentem mogla byc cala tablica i wtedy okno redukowalo `width * N` wartosci
/// (`MIN(a : 2)` nad `INTEGER[3]` mieszalo 6 liczb). Mieszalo to dwie rozne redukcje: po czasie
/// i po ROWNOLEGLYCH kanalach jednego rekordu. Nazwa tablicy nie jest nazwa pola - polami sa jej
/// elementy `a[0]`, `a[1]`, ... - wiec argumentem okna jest element, a `slot` wskazuje jeden
/// slot PLASKI rekordu zrodla.
/// Wartoscia rekordu moze byc takze WYRAZENIE (`MIN(a[0]*10 - a[1] : 3)`). Wtedy `program`
/// niesie jego tokeny, a `slot` nie znaczy nic: wartosc powstaje z przeliczenia programu na
/// payloadzie kazdego rekordu okna z osobna. Pusty `program` to argument bedacy golym polem -
/// szybka sciezka, ktora czyta slot wprost, bez ewaluatora.
///
/// `valueType` ustala KOMPILACJA (typ pola albo najszerszy typ pol wyrazenia, przepuszczony
/// przez reductionResultField), bo dla wyrazenia nie ma jednego wpisu deskryptora, z ktorego
/// mozna go odczytac w wykonaniu.
/// Obiekt klasy windowGroup powinien:
/// - [query.window-group.1~1] przechowywać strumień źródłowy, szerokość okna w rekordach, typ wartości redukcji oraz albo slot płaski pola źródła, albo program wyrażenia liczonego na każdym rekordzie okna.
/// - [query.window-group.2~1] traktować pusty program jako odczyt slotu wprost, a niepusty program jako wyrażenie, przy którym slot nie ma znaczenia.
/// - [query.window-group.3~1] przez sameAs uznawać dwie grupy za identyczne wyłącznie przy równym źródle, szerokości i typie wartości oraz równym slocie dla grup bez programu albo równych co do polecenia i wartości tokenach programu dla grup z programem.
struct windowGroup {
  std::string source;                      ///< strumien zrodlowy (nazwa fizyczna po ekspansji generatorow)
  int slot  = 0;                           ///< slot plaski pola w rekordzie zrodla; nieuzywany, gdy `program` niepusty
  int width = 0;                           ///< szerokosc okna w REKORDACH
  std::list<token> program;                ///< rachunek wartosci rekordu; pusty = goly odczyt slotu
  rdb::descFld valueType = rdb::RATIONAL;  ///< typ wartosci wchodzacych do redukcji

  [[nodiscard]] bool sameAs(const windowGroup &other) const {
    if (source != other.source || width != other.width || valueType != other.valueType) return false;
    if (program.empty() != other.program.empty()) return false;
    if (program.empty()) return slot == other.slot;
    return std::ranges::equal(program, other.program, [](const token &a, const token &b) {
      return a.getCommandID() == b.getCommandID() && a.getVT() == b.getVT();
    });
  }
};

/// Wynik JEDNEGO przejścia po oknie grupy - wszystkie cztery agregaty naraz.
///
/// Liczymy komplet, a nie tylko żądany agregat, bo przejście po oknie jest kosztem
/// dominującym, a same porównania są darmowe. Dzięki temu `MIN(cells:10)` i
/// `MAX(cells:10)` w jednej liście SELECT czytają historię źródła raz.
///
/// `count` liczy wartości NIE-NULL. NULL-e są pomijane (nie zerowane), a okno bez ani
/// jednej wartości daje NULL we wszystkich czterech polach - zasada brzegu: brak wyniku
/// nie jest zerem.
/// @requirements-exempt windowStats: Agregat wynikow redukcji okna; obliczenia wykonuje dataModel.
struct windowStats {
  rdb::descFldVT minValue{std::monostate{}};
  rdb::descFldVT maxValue{std::monostate{}};
  rdb::descFldVT sumValue{std::monostate{}};
  rdb::descFldVT avgValue{std::monostate{}};
  int count = 0;
};

/// Rodzaj zrodla deklarowanego (#346). Wynika ze slowa kluczowego DECLARE, nigdy ze sciezki;
/// przestarzale `FILE` parser tlumaczy na jeden z trzech rodzajow, wiec dalej silnik widzi tylko je.
enum class sourceKind : std::uint8_t { none, binFile, textFile, device };

/// Funkcja sourceKindKeyword powinna:
/// - [query.source-kind-keyword.1~1] zwracać słowo kluczowe RQL rodzaju źródła: "BINFILE", "TEXTFILE" albo "DEVICE", a dla sourceKind::none pusty napis.
///
/// @note Do komunikatow i ostrzezen.
inline std::string_view sourceKindKeyword(sourceKind kind) {
  switch (kind) {
    case sourceKind::binFile:
      return "BINFILE";
    case sourceKind::textFile:
      return "TEXTFILE";
    case sourceKind::device:
      return "DEVICE";
    case sourceKind::none:
      break;
  }
  return "";
}

/// Obiekt klasy query powinien:
/// - [query.query.1~1] przechowywać nazwę, interwał, plik źródła, schemat pól z programami, program FROM, grupy okien rekordowych, reguły, retencję, politykę magazynu oraz wyniki kompilacji: ogon, origin i liczność generatora.
///   Sprawdzenie kompilacji: test/UnitTest/test_contracts.cpp i test/UnitTest/test_compiler.cpp - dostep do skladowych query w testach kontraktow i kompilacji.
/// - [query.query.2~1] traktować węzeł z pustym programem FROM jako deklarację źródła, węzeł o nazwie z przedrostkiem STREAM_ jako strumień generowany przez kompilator, a węzeł o nazwie zaczynającej się od dwukropka jako dyrektywę kompilatora.
/// - [query.query.3~1] przez isReductionRequired zwracać prawdę, gdy program FROM zawiera więcej niż jeden operator strumieniowy, wliczając reduktory AVG, MIN, MAX i SUMC.
/// - [query.query.4~1] przez getDepStream zwracać nazwy strumieni z tokenów PUSH_STREAM programu FROM w kolejności programu.
/// - [query.query.5~1] przez descriptorStorage budować deskryptor magazynu z pól schematu, a dla deklaracji dołączać REF z plikiem i TYPE z rodzaju źródła (BINFILE, TEXTSOURCE, DEVICE), dla SELECT zaś RETENTION przy zdefiniowanej retencji oraz RETMEMORY i TYPE przy polityce z pojemnością (MEMORY albo profil dyrektywy :SUBSTRAT).
/// - [query.query.6~1] przez storageType zwracać typ z polityki tylko wtedy, gdy polityka go wybiera (pojemność różna od zera i nazwa inna niż DEFAULT), a w przeciwnym razie STORAGE z planu.
/// - [query.query.7~1] przez descriptorFrom budować deskryptor wejścia operatora FROM: dla reduktorów jedno pole o typie wyniku redukcji, dla przeplotu po jednym polu na wspolny slot plaski skladowych z szerszym typem, dla AGSE abs(długość) pól płaskich o najszerszym typie źródła, a dla pozostałych operatorów pola źródeł z niezerową długością.
/// - [query.query.8~1] przez reset czyścić identyfikator, nazwę pliku, schemat, program FROM i ostrzeżenia oraz przywracać wartości początkowe interwału, flag, rodzaju źródła, numeru wiersza deklaracji, generatora, policy, retention i timeoutSeconds.
/// - [query.query.9~1] traktować wywołanie getFieldIndex dla pola spoza schematu jako błąd w kodzie wołającego (rdb::fatal).
/// - [query.query.10~1] porządkować węzły według interwału operatorem <.
class query {
  void fillDescriptor(const std::list<field> &lSchemaVar, rdb::Descriptor &val, const std::string &id);

 public:
  explicit query(boost::rational<int> rInterval, std::string id);
  query();

  std::list<std::string> getFieldNamesList();

  std::string id;
  std::string filename;
  boost::rational<int> rInterval = 0;

  /// @brief Licznosc rodziny generatora strumieni; -1 = zwyczajny strumien, nie generator.
  ///
  /// Ustawia parser z zapisu `STREAM cell[24]`, kasuje compiler::expandStreamGenerators()
  /// przy tworzeniu kazdej z N instancji. Zadne zapytanie z nieujemna wartoscia tego pola nie
  /// moze przezyc pierwszego przebiegu kompilacji - dalsze przebiegi go nie znaja.
  ///
  /// UJEMNA wartosc znaczy „to nie jest generator", a nie „generator o zerowej licznosci".
  /// Rozroznienie jest konieczne, bo `STREAM cell[0]` jest bledem, ktory trzeba zglosic -
  /// przy wartowniku 0 zapis ten stalby sie po cichu zwyklym strumieniem `cell`.
  static constexpr int notAGenerator = -1;
  int generatorSize                  = notAGenerator;

  /// @brief Ogon strumienia: liczba początkowych slotów WŁASNEGO interwału, w których wynik nie jest jeszcze zdefiniowany.
  ///
  /// Zasada brzegu strumienia: te sloty NIE są rekordami - ani zerami, ani NULL-ami. Strumień po prostu jeszcze nie
  /// emituje, a system raportuje ogon. NULL jest zarezerwowany dla wartości pochłaniającej: danych oczekiwanych
  /// a nieobecnych albo wyniku nieistniejącego w zbiorze wartości (np. dzielenie przez zero). NULL nigdy nie oznacza
  /// „system przewiduje tu miejsce na dane”.
  ///
  /// Wyliczane dokładną arytmetyką wymierną przez compiler::computeStartupLatency().
  int startupLatency = 0;

  /// @brief Początek logiczny strumienia: indeks PIERWSZEGO rekordu, który w ogóle istnieje.
  ///
  /// Rekordy o indeksie mniejszym od origin nie powstają - nie dlatego, że jeszcze ich nie ma (od tego jest ogon),
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

  /// Rodzaj zrodla deklaracji; `none` dla SELECT i dyrektyw.
  sourceKind kind = sourceKind::none;
  /// Deklaracja zapisana przestarzala forma `FILE` - `kind` wybrala zamrozona regula ze sciezki.
  bool isDeprecatedFile = false;
  /// Wiersz pliku planu, od ktorego zaczyna sie deklaracja - do ostrzezenia o formie przestarzalej.
  size_t declarationLine = 0;
  /// Ostrzezenia K6 o przestarzalej notacji przyrostkowej (`s.avg`) w tym zapytaniu, w postaci
  /// gotowej do pokazania. Log parsera wycina Release (SPDLOG_ACTIVE_LEVEL), a fasada osadzona
  /// oddaje je hostowi jako DeprecationWarning (#476).
  std::vector<std::string> deprecatedNotations;
  /// Jawna klauzula `TIMEOUT` deklaracji DEVICE w sekundach (#347), wymierna jak interwal. Brak
  /// klauzuli to cos innego niz jawne 0: brak bierze `[sources] timeout_s` z retractor.toml, a jawne 0
  /// wylacza dodatnia wartosc z konfiguracji dla tego jednego zrodla. Pierwszenstwo rozstrzyga
  /// effectiveDeviceTimeout().
  std::optional<boost::rational<int>> timeoutSeconds;

  std::list<field> lSchema;
  std::list<token> lProgram;

  /// Grupy okien rekordowych tego zapytania - jedna pozycja na trojke
  /// (zrodlo, pole, szerokosc). Token WINDOW_* niesie indeks do tej tabeli.
  ///
  /// Tabela jest skladowa ZAPYTANIA, nie planu, i to jest wymog poprawnosci. Zapytania
  /// wedruja miedzy drzewami: compiler::importFrom() przenosi zapytanie ad hoc do zywego
  /// planu, a expandStreamGenerators() kopiuje szablon na N instancji. Indeks globalny
  /// przezylby te wedrowke tylko przy przenumerowaniu tabeli w kazdym z tych miejsc;
  /// tabela lokalna jedzie razem z zapytaniem i pozostaje spojna sama z siebie.
  ///
  /// Grupa niesie WSZYSTKIE cztery wartosci (min, max, sum, count) z jednego przejscia po
  /// oknie, wiec `MIN(cells:10)` i `MAX(cells:10)` w tej samej liscie SELECT czytaja okno
  /// raz - o to chodzi we wspolnym stanie dla agregatow o identycznym ksztalcie.
  std::vector<windowGroup> windowGroups;

  [[nodiscard]] bool hasWindowAggregates() const { return !windowGroups.empty(); }

  std::list<rule> lRules;

  rdb::retention_t retention            = rdb::retention_t{.segments = 0, .capacity = 0};  // Retention segments and capacity
  std::pair<std::string, size_t> policy = std::make_pair("DEFAULT", rdb::memoryFile::no_retention);
  std::string storage_policy            = "DEFAULT";

  /// Typ magazynu wyniku SELECT, ktory wybierze rdb::storage: TYPE z descriptorStorage, a bez niego
  /// STORAGE z planu.
  [[nodiscard]] std::string storageType() const;

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

/// Funkcja operator< powinna:
/// - [query.operator-less.1~1] porządkować węzły wyłącznie według interwału rInterval, rosnąco.
bool operator<(const query &lhs, const query &rhs);

/// Efektywny termin odczytu zrodla DEVICE i to, skad pochodzi (#347).
/// @requirements-exempt deviceTimeout: Wartosc terminu DEVICE i informacja o pochodzeniu ustawienia.
struct deviceTimeout {
  double seconds = 0.0;
  /// "RQL", "config", "default" albo "no-clock" - do wpisu w logu przy starcie epoki.
  std::string_view origin;
};

/// Funkcja effectiveDeviceTimeout powinna:
/// - [query.effective-device-timeout.1~1] w trybie bez zegara (noClock, `-f`) zwracać 0 s z pochodzeniem "no-clock".
/// - [query.effective-device-timeout.2~1] w przeciwnym razie zwracać jawną klauzulę TIMEOUT węzła z pochodzeniem "RQL", a bez niej configured z pochodzeniem "config", a bez obu 0 s z pochodzeniem "default".
///
/// @note #347. Bez zegara termin wynosi zawsze 0: sekundy rzeczywiste nie maja przelicznika na czas wirtualny,
///       a proba nieblokujaca zostawia DEVICE uzytecznym w szybkich testach. configured to `[sources] timeout_s`.
deviceTimeout effectiveDeviceTimeout(const query &qry, std::optional<double> configured, bool noClock);

/// Funkcja GetArgs powinna:
/// - [query.get-args.1~1] dla programu FROM z jednym tokenem zwracać jego napis jako pierwszy argument, pusty drugi i ten sam token jako operator.
/// - [query.get-args.2~1] dla dwóch tokenów zwracać napis pierwszego jako pierwszy argument, pusty drugi i drugi token jako operator.
/// - [query.get-args.3~1] dla trzech tokenów zwracać napisy dwóch pierwszych jako argumenty i trzeci token jako operator.
/// - [query.get-args.4~1] kończyć proces przez RDB_ASSERT dla programu dłuższego niż trzy tokeny.
///
/// @note Wolaja ja wylacznie przebiegi kompilatora po extractIntermediateStreams() i wykonanie planu
///       skompilowanego, gdzie program FROM ma 1..3 tokeny.
std::tuple<std::string, std::string, token> GetArgs(std::list<token> &prog);
