#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "retractor/lib/bus.hpp"

/// @brief Rozstrzyganie instancji docelowej dla klienta `xqry`.
///
/// Funkcje są CZYSTE: pracują na gotowej migawce magistrali i nie dotykają IPC. Dzięki temu
/// reguły routingu da się sprawdzić testem jednostkowym, bez startowania serwerów i bez
/// pamięci dzielonej - a to jest właśnie ta część, w której łatwo o pomyłkę.
///
/// Zasada nadrzędna: rozstrzygnięcie NIGDY nie odpytuje serwerów. Osierocony segment w
/// /dev/shm jest nieodróżnialny od żywego aż do wyczerpania budżetu klienta (3 s), więc
/// szukanie strumienia przez odpytywanie kosztowałoby N × 3 s dokładnie w najczęstszym
/// przypadku - literówce w nazwie. Żywotność instancji rozstrzyga `/proc`, nie timeout.
namespace routing {

enum class Status : std::uint8_t {
  Resolved,        ///< nazwa instancji rozstrzygnięta (pusta => instancja historyczna)
  StreamNotFound,  ///< żadna żywa instancja nie serwuje tego strumienia
  Ambiguous,       ///< komenda nie wskazuje instancji, a żywych jest więcej niż jedna
  CrossServer,     ///< zapytanie ad-hoc sięga strumieni należących do różnych instancji
};

/// @requirements-exempt Resolution: Wynik routingu; wybor instancji realizuja funkcje modulu.
struct Resolution {
  Status status{Status::Resolved};
  std::string serverName;  ///< znaczące tylko dla Resolved
  std::string detail;      ///< gotowa treść komunikatu dla operatora
};

/// @requirements-exempt NamespaceGroup: Dane grupy instancji nalezacych do jednej przestrzeni nazw.
struct NamespaceGroup {
  std::string name;  ///< pusta => magistrala domyslna
  std::vector<bus::InstanceInfo> instances;
};

/// Funkcja instanceLabel powinna:
/// - [serverrouting.instance-label.1~1] zwracać nazwę instancji bez zmian, a dla pustej nazwy (instancja bez `--name`) napis "(unnamed)".
///
/// @note Jedno pole bez spacji, wiec wyjscie `--bus` zostaje kolumnowo rozbieralne.
[[nodiscard]] std::string instanceLabel(std::string_view name);

/// Funkcja extractSourceStreams powinna:
/// - [serverrouting.extract-source-streams.1~1] wydzielać identyfikatory w składni RQL ID (`[A-Za-z][A-Za-z_$0-9]*`), pomijając literały w apostrofach (z ucieczką odwrotnym ukośnikiem), komentarze wierszowe (dwa ukośniki) do końca wiersza i zagnieżdżone komentarze blokowe (ukośnik z gwiazdką).
/// - [serverrouting.extract-source-streams.2~1] zwracać identyfikator stojący bezpośrednio za słowem ON (strumień reguły RULE).
/// - [serverrouting.extract-source-streams.3~1] po słowie FROM zwracać każdy kolejny identyfikator z wyjątkiem reduktorów MIN, MAX, AVG i SUMC oraz identyfikatora stojącego bezpośrednio po kropce.
/// - [serverrouting.extract-source-streams.4~1] kończyć analizę na pierwszym słowie FILE, RETENTION, VOLATILE albo STORAGE po FROM.
///   Zakres testow: `serverRouting.sourceStreamsStopAtFirstTerminatorAfterFrom`; FILE przed FROM bez przypadku (wystepuje tylko w przestarzalym DECLARE ... FILE).
/// - [serverrouting.extract-source-streams.5~1] rozpoznawać słowa kluczowe wyłącznie w pisowni całej wielkimi albo całej małymi literami, jak w `RQL.g4`.
/// - [serverrouting.extract-source-streams.6~1] zwracać nazwy w kolejności wystąpienia, z powtórzeniami.
///
/// @note Pisownia mieszana (`Min`, `From`) jest w RQL.g4 zwykla nazwa strumienia; skladanie do malych liter
///       gubiloby jej wlasciciela. Identyfikatory, ktore nie sa strumieniami, odsiewa dopiero wolajacy przez
///       migawke magistrali.
[[nodiscard]] std::vector<std::string> extractSourceStreams(std::string_view query);

/// Funkcja forStream powinna:
/// - [serverrouting.for-stream.1~1] zwracać Resolved z nazwą pierwszej w kolejności migawki instancji, która serwuje strumień.
/// - [serverrouting.for-stream.2~1] zwracać StreamNotFound z komunikatem "<strumień>: stream not found on any live instance", gdy żadna instancja go nie serwuje.
/// - [serverrouting.for-stream.3~1] nie odpytywać serwerów.
///   Nietestowalne: brak odpytywania wynika z sygnatury (funkcja dostaje tylko migawke magistrali, bez uchwytu IPC) i z braku wywolan IPC w serverRouting.cpp; posrednio: testy serverRouting.* licza wynik bez zadnego serwera i bez segmentu.
///
/// @note Magistrala pilnuje rozlacznosci nazw, wiec pierwsze trafienie jest jedynym. StreamNotFound musi
///       zostac kodem "nie ma takiego strumienia", a nie "serwer milczy" (issue_215).
[[nodiscard]] Resolution forStream(const std::vector<bus::InstanceInfo> &instances, std::string_view stream);

/// Funkcja forAdHoc powinna:
/// - [serverrouting.for-ad-hoc.1~1] brać nazwy z extractSourceStreams(query) i zwracać Resolved z instancją, gdy wszystkie nazwy serwowane przez żywe instancje należą do tej jednej instancji.
/// - [serverrouting.for-ad-hoc.2~1] zwracać CrossServer z listą par `strumień@instancja` bez powtórzeń, gdy nazwy należą do więcej niż jednej instancji.
/// - [serverrouting.for-ad-hoc.3~1] zwracać Ambiguous z listą żywych instancji i podpowiedzią `--server <name>`, gdy żadna nazwa nie należy do żywej instancji.
/// - [serverrouting.for-ad-hoc.4~1] pomijać nazwy, których nie serwuje żadna instancja, i nie odpytywać serwerów.
///   Zakres testow: `serverRouting.adHocSkipsUnknownNamesWhenOwnerIsUnique`; brak odpytywania serwerow wynika z sygnatury (tylko migawka, bez uchwytu IPC), nie z asercji.
///
/// @note Rozglaszanie jest wykluczone - `getAdHoc` modyfikuje PLAN serwera, wiec trafienie w niewlasciwa
///       instancje jest trwalym skutkiem ubocznym, a zgadywanie zmienialoby plan przypadkowego serwera.
[[nodiscard]] Resolution forAdHoc(const std::vector<bus::InstanceInfo> &instances, std::string_view query);

/// Funkcja forSingleTarget powinna:
/// - [serverrouting.for-single-target.1~1] przy braku żywych instancji zwracać Resolved z pustą nazwą (tryb historyczny).
/// - [serverrouting.for-single-target.2~1] przy dokładnie jednej instancji zwracać Resolved z jej nazwą.
/// - [serverrouting.for-single-target.3~1] przy więcej niż jednej instancji zwracać Ambiguous z ich liczbą, listą i podpowiedzią `--server <name>`.
///
/// @note Dla komend dotyczacych calej instancji (-k, -d, -l). Zepsuta albo pusta magistrala nie moze
///       unieruchomic klienta.
[[nodiscard]] Resolution forSingleTarget(const std::vector<bus::InstanceInfo> &instances);

/// Funkcja describe powinna:
/// - [serverrouting.describe.1~1] dla pustej migawki zwracać pustą listę wierszy.
/// - [serverrouting.describe.2~1] zwracać wiersz nagłówka SERVER | PID | MODE | QUERY | STREAMS, wiersz kresek i wiersze instancji posortowane po instanceLabel, a na końcu legendę liter trybów zaczynającą się od "MODE:".
/// - [serverrouting.describe.3~1] dopełniać kolumny SERVER, PID, MODE i QUERY do szerokości najszerszej wartości; szerokość kolumny STREAMS, mierzona pojedynczą nazwą strumienia, wyznacza tylko linię kresek.
/// - [serverrouting.describe.4~1] wypisywać strumienie po jednym na linię: pierwszy w wierszu instancji, kolejne w liniach kontynuacji z pustymi kolumnami po lewej, a dla instancji bez strumieni "-".
/// - [serverrouting.describe.5~1] w kolumnie MODE podawać litery ustawionych trybów w kolejności R, F, U, M, X, S, a bez żadnego "N".
/// - [serverrouting.describe.6~1] bezwzględną ścieżkę zapytania skracać do `.../<katalog>/<plik>`, gdy to ją skraca, ścieżkę względną zostawiać bez zmian, a brak ścieżki pokazywać jako "-".
///
/// @note Kolejnosc slotow w segmencie zalezy od kolejnosci startow, a wyjscie ma byc powtarzalne. Legenda
///       nie pasuje do wzorca `^<nazwa> |`, wiec skrypty dopasowujace wiersze instancji jej nie widza.
[[nodiscard]] std::vector<std::string> describe(const std::vector<bus::InstanceInfo> &instances);

/// Funkcja describeNamespacesYaml powinna:
/// - [serverrouting.describe-namespaces-yaml.1~1] zwracać dokument YAML zaczynający się od "---" i "apiVersion: xqry/v1", a dla pustej listy grup kończący się "servers: []".
/// - [serverrouting.describe-namespaces-yaml.2~1] dla grup w podanej kolejności wypisywać ich instancje posortowane po instanceLabel: name, namespace (null dla magistrali domyślnej, a w pozostałych przypadkach nazwa w cudzysłowie), pid, modes (litery jak w describe), pełną ścieżkę query w cudzysłowie (z ucieczką `\` i `"`), gdy jest niepusta, oraz listę streams albo "streams: []".
/// - [serverrouting.describe-namespaces-yaml.3~1] nie wypisywać legendy liter trybów.
///   Zakres testow: `serverRouting.describeNamespacesYamlKeepsOrderAndFullQueryPath`, `serverRouting.describeNamespacesYamlKeepsGroupOrderAndEscapesQuery`, `serverRouting.describeNamespacesYamlDescribesEmptyBusAndEmptyFields`; dokladna liczba linii wyklucza legende.
/// - [serverrouting.describe-namespaces-yaml.4~1] dla niepustej listy grup, z których żadna nie ma instancji, wypisywać samo "servers:".
///
/// @note Lista wszystkich magistral dla `xqry --bus -y`. Jedyny wolajacy (qryLauncher) pomija grupy bez
///       instancji, wiec przypadek "servers:" bez listy nie wystepuje. Konsument formatu ma dostac dokument,
///       nie nic. Sciezka jest pelna, bo odbiorca ma ja moc otworzyc, i cytowana, bo moze zawierac dwukropek
///       albo spacje.
[[nodiscard]] std::vector<std::string> describeNamespacesYaml(const std::vector<NamespaceGroup> &groups);

}  // namespace routing
