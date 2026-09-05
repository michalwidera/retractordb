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
/// pamięci dzielonej — a to jest właśnie ta część, w której łatwo o pomyłkę.
///
/// Zasada nadrzędna: rozstrzygnięcie NIGDY nie odpytuje serwerów. Osierocony segment w
/// /dev/shm jest nieodróżnialny od żywego aż do wyczerpania budżetu klienta (3 s), więc
/// szukanie strumienia przez odpytywanie kosztowałoby N × 3 s dokładnie w najczęstszym
/// przypadku — literówce w nazwie. Żywotność instancji rozstrzyga `/proc`, nie timeout.
namespace routing {

enum class Status : std::uint8_t {
  Resolved,        ///< nazwa instancji rozstrzygnięta (pusta => instancja historyczna)
  StreamNotFound,  ///< żadna żywa instancja nie serwuje tego strumienia
  Ambiguous,       ///< komenda nie wskazuje instancji, a żywych jest więcej niż jedna
  CrossServer,     ///< zapytanie ad-hoc sięga strumieni należących do różnych instancji
};

struct Resolution {
  Status status{Status::Resolved};
  std::string serverName;  ///< znaczące tylko dla Resolved
  std::string detail;      ///< gotowa treść komunikatu dla operatora
};

/// Nazwa instancji w komunikacie; instancja bez `--name` jako "(unnamed)" — jedno pole,
/// bez spacji, więc wyjście `--bus` zostaje kolumnowo rozbieralne.
[[nodiscard]] std::string instanceLabel(std::string_view name);

/// Nazwy strumieni występujące w wyrażeniu FROM zapytania SELECT. Lekser zachowuje składnię
/// RQL ID (`[A-Za-z][A-Za-z_$0-9]*`), pomija napisy i komentarze oraz odróżnia reduktory
/// MIN/MAX/AVG/SUMC i agregator po kropce od nazw źródeł.
///
/// Słowa kluczowe rozpoznaje dokładnie w dwóch pisowniach z `RQL.g4` (`'FROM'|'from'`), bo
/// tylko te dwie są tam słowami kluczowymi — `Min` czy `From` to zwykłe nazwy strumieni.
[[nodiscard]] std::vector<std::string> extractSourceStreams(std::string_view query);

/// Właściciel strumienia. Magistrala pilnuje rozłączności nazw, więc właściciel jest co
/// najwyżej jeden i pierwsze trafienie jest jedynym.
[[nodiscard]] Resolution forStream(const std::vector<bus::InstanceInfo> &instances, std::string_view stream);

/// Właściciel zapytania ad-hoc: wszystkie rozpoznane nazwy strumieni muszą należeć do jednej
/// instancji. Rozgłaszanie jest wykluczone — `getAdHoc` modyfikuje PLAN serwera, więc trafienie
/// w niewłaściwą instancję nie jest pomyłką do powtórzenia, tylko trwałym skutkiem ubocznym.
[[nodiscard]] Resolution forAdHoc(const std::vector<bus::InstanceInfo> &instances, std::string_view query);

/// Komenda dotycząca całej instancji (-k, -d, -y, -l): rozstrzygalna tylko wtedy, gdy żywa
/// instancja jest dokładnie jedna albo nie ma żadnej (wtedy nazwa pusta = tryb historyczny).
[[nodiscard]] Resolution forSingleTarget(const std::vector<bus::InstanceInfo> &instances);

/// Tabela wypisywana przez `xqry --bus`, posortowana po nazwie instancji. Bezwzgledna
/// sciezka zapytania jest prezentowana jako `.../<katalog>/<plik>`, a strumienie po przecinku.
/// Szerokosc kazdej kolumny wynika z najszerszej wartosci, wiec nazwy z `--autoname` nie
/// rozjezdzaja wiersza. Kolumna MODE niesie litery trybow pracy instancji (R/F/U/M/X/S,
/// N = zwykly), a ostatni wiersz tabeli to ich legenda.
[[nodiscard]] std::vector<std::string> describe(const std::vector<bus::InstanceInfo> &instances);

}  // namespace routing
