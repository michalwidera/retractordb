#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "constants.hpp"
#include "rdb/sizeLimits.hpp"

/// @brief Magistrala xrdbbus: wspolny obszar wykrywania instancji xretractor i egzekwowania
///        rozlacznosci zasobow -- nazw strumieni i plikow magazynu -- miedzy serwerami
///        na jednej maszynie.
///
/// Obszar wspolny zawiera WYLACZNIE typy POD i tablice o stalym rozmiarze -- zadnego
/// alokatora i zadnego kontenera Boosta. Powod jest konkretny: serwer zabity w trakcie
/// wstawiania do kontenera z alokatorem zostawia niespojna sterte w pamieci dzielonej,
/// ktorej nie da sie naprawic. Przy stalych slotach naprawa niezmiennika to jedna
/// operacja -- uniewaznic slot, ktorego dotyczyl przerwany zapis.
///
/// Trzy wlasnosci ukladu:
///   1. Jeden pisarz na slot -- instancja pisze wylacznie swoj wlasny slot.
///   2. Odczyt bez blokady -- seqlock, wiec czytelnik nie zaciera sie o kondujacy serwer.
///   3. Muteks (robust, pshared) wylacznie przy roszczeniu i zwalnianiu slotu.
///
/// Zywotnosc slotu: /proc/<pid> istnieje ORAZ startTime sie zgadza. Sam kill(pid, 0) nie
/// wystarcza, bo PID-y sa reuzywane. Slot martwy jest wolny; kasuje go ten, kto to
/// zauwazy -- bez demona i bez heartbeatow. Proces zombie liczy sie jako martwy, patrz
/// isProcessAlive ponizej.
namespace bus {

/// Nazwa segmentu w /dev/shm.
///
/// Kasowania pilnuje protokol obecnosci. Kto segment mapuje, trzyma LOCK_SH na pliku
/// <ipc::kMachineLockDir>/<nazwa segmentu>.lock - zajmuje go PRZED otwarciem segmentu, zwalnia
/// po zamknieciu odwzorowania. Segment kasuje wylacznie ten, kto zajmie ten plik LOCK_EX, czyli gdy
/// nie mapuje go nikt: ostatni wychodzacy (Bus::~Bus) albo sprzatacz po procesach zabitych
/// (sweepAbandonedSegments). Samo "sprawdz, czy nikt nie zyje, i skasuj" byloby wyscigiem, w ktorym
/// dwie instancje koncza na DWOCH segmentach, kazda widzac tylko siebie; blokada obecnosci go
/// zamyka, a regule sprawdzania i-wezla po zajeciu blokady opisuje lockFile.hpp.
///
/// NAZWA NIESIE WERSJE UKLADU i przy KAZDEJ zmianie ukladu slotu musi isc w gore razem
/// z kLayoutVersion. Powod jest wdrozeniowy: segment o starym ukladzie zostaje w /dev/shm po
/// podmianie binarki, a instancja, ktora odmowi sie do niego podlaczyc, startuje BEZ egzekwowania
/// rozlacznosci nazw -- czyli awaria jest cicha az do pierwszej kolizji. Z wersja w nazwie nowa
/// binarka po prostu zaklada wlasny segment, a stary zostaje nieuzywanym smieciem do restartu
/// maszyny. Sprzatacz innych wersji nie rusza: binarki sprzed protokolu obecnosci (v5 i starsze)
/// blokady nie biora, wiec jej brak nic o nich nie mowi.
///
/// Podkreslenie, nie kropka: obiekty IPC instancji nazywaja sie "<obiekt>.<nazwa instancji>", wiec
/// "xrdbbus.v2" wygladalby jak obiekt instancji o nazwie "v2" i wpadl pod wzorce sprzatajace
/// postaci /dev/shm/*.<nazwa>.
inline constexpr std::string_view kSegmentName = "xrdbbus_v7";

/// Funkcja segmentName powinna:
/// - [bus.segment-name.1~1] zwracać nazwę segmentu dla bieżącego uruchomienia: kSegmentName + "_" + przestrzeń nazw z servername::environmentNamespace(), gdy przestrzeń jest niepusta i poprawna według servername::isValid, a w przeciwnym razie samo kSegmentName.
///
/// @note Rozlacznosc nazw strumieni jest wlasnoscia CALEJ maszyny, wiec dwa zestawy testow integracyjnych z
///       tymi samymi nazwami (`core0`, `dst`, `str1`) nie moga biec na jednym segmencie - drugi start odpada na
///       ClaimStatus::Conflict. Niepoprawna przestrzen odrzucaja launchery; tutaj chroni pozostalych wolajacych.
///       Podkreslenie, nie kropka: kropka nalezy do obiektow instancji i wpadalaby pod wzorce sprzatajace.
[[nodiscard]] std::string segmentName();

/// Funkcja segmentNames powinna:
/// - [bus.segment-names.1~1] zwracać posortowane nazwy segmentów bieżącej wersji układu odczytane z plików `<segment>.lock` w ipc::kMachineLockDir: dokładnie kSegmentName albo kSegmentName + "_" + nazwa poprawna według servername::isValid.
///   Zakres testow: `BusSegmentNames.ListsOnlyCurrentLayoutPresenceFiles`; samo kSegmentName (plik /tmp/xrdbbus_v7.lock) bez przypadku - nalezy do dzialajacych serwerow produkcyjnych, test go nie zaklada.
/// - [bus.segment-names.2~1] kończyć przegląd bez błędu, gdy katalogu nie da się otworzyć albo czytać.
///   Nietestowalne: katalog jest staly (ipc::kMachineLockDir), bez parametru, wiec testu nie da sie skierowac na katalog nieczytelny; ta sama petla directory_iterator z error_code w lockfile::sweep konczy sie zerem w `BusPresence.SweepLeavesOtherLayoutVersions`.
/// - [bus.segment-names.3~1] nie sprawdzać, czy segment albo żywa instancja nadal istnieje.
///
/// @note Samo znalezienie pliku obecnosci niczego nie dowodzi; czytelnik sprawdza to przez Bus.
[[nodiscard]] std::vector<std::string> segmentNames();

/// Funkcja sweepAbandonedSegments powinna:
/// - [bus.sweep-abandoned-segments.1~1] przez lockfile::sweep w machineLockDir zajmować porzucone pliki obecności segmentów bieżącej wersji układu (nazwy jak w segmentNames), pod tą blokadą usuwać segment o nazwie pliku bez ".lock" przez ipc::security::removeOwned, a potem plik obecności.
/// - [bus.sweep-abandoned-segments.2~1] nie ruszać plików obecności innych wersji, plików trzymanych przez kogokolwiek ani plików innych kont.
///   Zakres testow: `BusPresence.SweepLeavesOtherLayoutVersions`, `BusPresence.SweepRemovesOnlyUnmappedSegments`; pliki innych kont bez przypadku - wymagaja drugiego konta (root i chown), pomijanie innego UID nalezy do lockfile::sweep.
/// - [bus.sweep-abandoned-segments.3~1] zwracać liczbę usuniętych plików obecności, także tych, przy których removeOwned odmówił usunięcia segmentu.
///   Zakres testow: `BusPresence.SweepRemovesOnlyUnmappedSegments`, `BusPresence.SweepCountsPresenceWhoseSegmentRemovalWasRefused` (tylko RDB_HAS_BOOST_POSIX_SHM: obiekt podmieniony na dowiazanie).
///
/// @note Sprzata pozostalosci po procesach zabitych, ktore nie zdazyly posprzatac jako ostatni wychodzacy.
///       Inne wersje maja binarki sprzed protokolu obecnosci, ktore blokady nie biora, wiec jej brak nic o nich
///       nie mowi. Katalog inny niz domyslny podaja tylko testy.
std::size_t sweepAbandonedSegments(std::string_view machineLockDir = ipc::kMachineLockDir);

/// Funkcja segmentBytes powinna:
/// - [bus.segment-bytes.1~1] zwracać rozmiar całego segmentu magistrali w bajtach, bez ujawniania jego układu.
///
/// @note Segment jest tworzony w calosci i od razu zerowany, wiec to miejsce ZAJETE w /dev/shm, a nie
///       rezerwacja rosnaca z uzyciem. Sluzy wycenie budzetu pamieci dzielonej (shmBudget).
[[nodiscard]] std::size_t segmentBytes();

/// Pojemnosci ukladu. Przekroczenie ktoregokolwiek limitu jest bledem startu, a nie cichym
/// zawieszeniem gwarancji rozlacznosci -- slot z obcieta lista strumieni nie moglby juz
/// odpowiadac na pytanie "czyja jest ta nazwa".
///
/// Dlugosc nazwy strumienia jest zwiazana z budzetem kompilatora: substratNameBudget_C
/// (compiler.cpp) wynosi 200 znakow, a nazwy dluzsze compiler::composeStreamName zastepuje
/// skrotem. Pole krotsze niz ten budzet wywracalo start planow z szerokim FROM -- nazwa
/// STREAM_ADD_STREAM_ADD_..._str01_..._str12 z it_wide_from_names ma ponad 130 znakow.
///
/// Liczba strumieni ma zapas 17 nad najwiekszym planem korpusu artykulu (K6c W4_Q32, 131 wezlow);
/// w repozytorium najwiekszy skompilowany plan (test/IntegrationTest/optimizer_ablation) ma 53 wezly.
/// Wartosc stoi w rdb/sizeLimits.hpp, bo te sama granice sprawdzaja parser (rozmiar generatora)
/// i kompilator (plan po rozwinieciu generatorow).
inline constexpr std::size_t kMaxSlots         = 32;
inline constexpr std::size_t kMaxStreams       = rdb::limits::kMaxPlanStreams;
inline constexpr std::size_t kStreamNameSize   = 208;  ///< z terminatorem => nazwa do 207 znakow
inline constexpr std::size_t kInstanceNameSize = 40;   ///< servername::kMaxLength (32) + zapas
inline constexpr std::size_t kQueryFileSize    = 256;  ///< z terminatorem => sciezka do 255 znakow
inline constexpr std::size_t kUnitNameSize     = 128;  ///< nazwa jednostki systemd; limit systemd to 256, tu z zapasem
inline constexpr std::size_t kCounterPathSize  = 256;  ///< znormalizowana sciezka licznika :ROTATION

/// Magazynow jest co najwyzej tyle, co strumieni: kazdy zapisywalny wezel planu ma dokladnie
/// jeden plik danych.
inline constexpr std::size_t kMaxStores = kMaxStreams;

/// Przekroczenie limitu ma DWA rozne skutki, i podzial nie jest dowolny.
///
/// Pola ROZSTRZYGAJACE rozlacznosc -- nazwa strumienia, sciezka licznika i sciezka magazynu -- to odmowa startu:
/// obciety napis zrownalby dwa rozne zasoby albo rozdzielil jeden, czyli zawiesilby gwarancje
/// po cichu. Pola INFORMACYJNE -- nazwa jednostki systemd i plik zapytan -- sa obcinane, bo
/// odmowa startu z powodu dlugiej nazwy unitu byloby lekarstwem gorszym od choroby; obciecie
/// jest wtedy zapisywane ostrzezeniem, zeby przyciety identyfikator w `xqry --bus` nie
/// wygladal na prawdziwy.

/// Tryby pracy instancji, publikowane w slocie jako maska bitowa. Sluza WYLACZNIE prezentacji
/// (`xqry --bus`): magistrala nie podejmuje na ich podstawie zadnej decyzji, wiec instancja
/// starszej binarki, ktora zglosi zero bitow, jest opisana jako zwykla -- a nie odrzucona.
///
/// Tryby nie sa rozlaczne: --realtime idzie w parze z --service, a --no-clock z --until-eof.
/// Wyklucza sie tylko para --realtime / --no-clock, i to juz na poziomie argumentow.
namespace mode {
inline constexpr std::uint32_t kRealTime  = 1U << 0;  ///< --realtime  (R)
inline constexpr std::uint32_t kNoClock   = 1U << 1;  ///< --no-clock  (F)
inline constexpr std::uint32_t kUntilEof  = 1U << 2;  ///< --until-eof (U)
inline constexpr std::uint32_t kLoopLimit = 1U << 3;  ///< --llimitqry z limitem innym niz "bez limitu" (M)
inline constexpr std::uint32_t kXqryWait  = 1U << 4;  ///< --xqrywait  (X)
inline constexpr std::uint32_t kService   = 1U << 5;  ///< --service albo praca jako jednostka systemd (S)
}  // namespace mode

/// Skrot sciezki pliku magazynu -- to, co trafia do slotu ZAMIAST samej sciezki.
///
/// Powod jest pojemnosciowy i mierzalny. Sciezek jest do kMaxStores na slot, wiec tablica
/// napisow po 256 bajtow (aktywna i zarezerwowana) urosla by slot z 54 KiB do 117 KiB, a segment
/// z 1,65 MiB do 3,65 MiB. Testy integracyjne trzymaja pule SZESNASTU segmentow (patrz
/// test/IntegrationTest/CMakeLists.txt) przeciwko /dev/shm, ktory w kontenerze CI ma domyslnie
/// 64 MB -- 58 MiB samych magistrali zaczelo by wypychac kolejki IPC, a przepelnienie /dev/shm
/// NIE wywraca startu: daje ClaimStatus::Unavailable, czyli po cichu zdejmuje egzekwowanie
/// rozlacznosci. Skrot o stalej dlugosci zamyka slot na 58 KiB (segment 1,78 MiB).
///
/// Diagnostyka na tym nie traci: kolidujaca sciezka wypisywana operatorowi jest sciezka WLASNA
/// -- ta, o ktora wlasnie prosilismy -- a te mamy w calosci. Slot cudzej instancji sluzy
/// wylacznie do porownania.
///
/// Porownywana jest SCIEZKA JAKO NAPIS, dokladnie jak przy liczniku :ROTATION: normalizuje ja
/// wolajacy (absolutePathOf), a magistrala nie zaglada do systemu plikow. Dowiazanie symboliczne
/// prowadzace do tego samego pliku pod inna nazwa nie jest wiec wykrywane -- ani tu, ani tam.
///
/// Cena skrotu: kolizja dwoch ROZNYCH sciezek bylaby falszywa odmowa startu. Przy 128 bitach
/// i skali rzedu tysiecy sciezek na maszyne prawdopodobienstwo jest rzedu 1e-31, czyli nizsze
/// niz przeklamanie samej pamieci.
/// @requirements-exempt StoreDigest: Para slow skrotu sciezki; porownanie wartosci jest domyslne.
struct StoreDigest {
  std::uint64_t high{0};
  std::uint64_t low{0};

  [[nodiscard]] friend bool operator==(const StoreDigest &, const StoreDigest &) = default;
};

/// Funkcja storeDigest powinna:
/// - [bus.store-digest.1~1] zwracać skrót 128-bitowy: `low` jako 64-bitowy FNV-1a ścieżki, a `high` jako FNV-1a ścieżki kontynuowany od stanu po soli "rdb-store".
/// - [bus.store-digest.2~1] dawać dla tej samej ścieżki ten sam skrót w każdym procesie, niezależnie od biblioteki standardowej.
///   Zakres testow: `BusStoreDigest.MatchesFnv1aReferenceValues`; wartosci wzorcowe wpisane w test, policzone niezaleznie.
/// - [bus.store-digest.3~1] traktować ścieżkę jako napis, bez normalizacji i bez zaglądania do systemu plików.
///
/// @note Skrot jedzie do pamieci dzielonej i porownuja go ROZNE procesy, wiec std::hash odpada. Dwie polowy
///       licza sie z dwoch roznych wiadomosci (druga z sola), zeby nie byly ze soba powiazane.
[[nodiscard]] StoreDigest storeDigest(std::string_view path);

/// Opis jednej zywej instancji odczytany z magistrali.
/// @requirements-exempt InstanceInfo: Migawka danych instancji odczytanych z magistrali.
struct InstanceInfo {
  std::string name;  ///< pusta => instancja historyczna (bez --name)
  std::int32_t pid{0};
  std::string queryFile;
  std::string unit;         ///< nazwa jednostki systemd; pusta => zwykly proces
  std::string counterPath;  ///< sciezka licznika :ROTATION; pusta => plan bez rotacji
  std::uint32_t modes{0};   ///< maska bus::mode::*; 0 => tryb zwykly
  std::vector<std::string> streams;
  /// Skroty sciezek plikow magazynu zapisywanych przez ten plan. NIE jest to lista rownolegla
  /// do `streams`: deklaracje (zrodla tylko do odczytu) i strumienie MEMORY nie maja w niej
  /// wpisu, a `FILE` zrywa zwiazek nazwy ze sciezka.
  std::vector<StoreDigest> storeDigests;
};

/// Wlasciciel nazwy strumienia znaleziony w migawce magistrali.
/// @requirements-exempt StreamOwner: Dane wlasciciela kolidujacej nazwy strumienia.
struct StreamOwner {
  std::string stream;    ///< kolidujaca nazwa
  std::string instance;  ///< wlasciciel; pusta => instancja bezimienna
  std::int32_t pid{0};
};

/// Wlasciciel pliku licznika rotacji znaleziony w migawce magistrali.
/// @requirements-exempt CounterOwner: Dane wlasciciela kolidujacego pliku licznika rotacji.
struct CounterOwner {
  std::string path;
  std::string instance;
  std::int32_t pid{0};
};

/// Wlasciciel pliku magazynu znaleziony w migawce magistrali. `path` jest sciezka Z ZAPYTANIA
/// wolajacego -- slot niesie sam skrot, wiec to jedyna postac, ktora da sie pokazac czlowiekowi.
/// @requirements-exempt StoreOwner: Dane wlasciciela kolidujacej sciezki magazynu.
struct StoreOwner {
  std::string path;
  std::string instance;
  std::int32_t pid{0};
};

/// Funkcja findForeignOwner powinna:
/// - [bus.find-foreign-owner.1~1] pomijać instancje o nazwie równej selfName, przy pustej selfName - wszystkie instancje bezimienne.
/// - [bus.find-foreign-owner.2~1] zwracać pierwszą w kolejności migawki pozostałą instancję, która ma którąkolwiek nazwę z streams, z tą nazwą (pierwszą w kolejności strumieni instancji), nazwą instancji i pid.
/// - [bus.find-foreign-owner.3~1] zwracać nullopt, gdy żadna inna instancja nie ma nazwy z streams.
/// - [bus.find-foreign-owner.4~1] działać wyłącznie na podanej migawce, bez IPC i bez kontaktu z serwerami.
///   Zakres testow: `BusForeignOwner.ReportsOwnerOutsideSelf`, `BusForeignOwner.DisjointSetAndEmptyInputsPass`; migawka z wymyslonymi pid bez segmentu i serwerow, brak IPC wynika z sygnatury, nie z asercji.
///
/// @note Sprawdza zestaw zapytan PRZED dostarczeniem go dzialajacemu serwisowi: instancje docelowa trzeba
///       pominac, bo restart i tak zwalnia jej slot.
[[nodiscard]] std::optional<StreamOwner> findForeignOwner(const std::vector<InstanceInfo> &instances, std::string_view selfName,
                                                          const std::vector<std::string> &streams);

/// Funkcja findForeignCounterOwner powinna:
/// - [bus.find-foreign-counter-owner.1~1] dla pustej counterPath zwracać nullopt.
/// - [bus.find-foreign-counter-owner.2~1] zwracać pierwszą w kolejności migawki instancję o nazwie innej niż selfName, której ścieżka licznika `:ROTATION` jest napisowo równa counterPath, z tą ścieżką, nazwą instancji i pid, a nullopt, gdy takiej nie ma.
///
/// @note Pusta sciezka nie rosci zasobu. Instancja docelowa jest pomijana, bo dostarczenie planu zastepuje
///       jej dotychczasowy slot po restarcie.
[[nodiscard]] std::optional<CounterOwner> findForeignCounterOwner(const std::vector<InstanceInfo> &instances,
                                                                  std::string_view selfName, std::string_view counterPath);

/// Funkcja findForeignStoreOwner powinna:
/// - [bus.find-foreign-store-owner.1~1] dla kolejnych ścieżek z stores liczyć storeDigest i zwracać pierwszą ścieżkę, której skrót występuje w storeDigests instancji o nazwie innej niż selfName, z tą ścieżką wołającego, nazwą instancji i pid.
/// - [bus.find-foreign-store-owner.2~1] zwracać nullopt, gdy żadna inna instancja nie ma skrótu żadnej ze ścieżek.
///
/// @note Chroni to, czego nazwa strumienia nie chroni: `FILE` odrywa nazwe pliku od nazwy zapytania, wiec dwa
///       plany o ROZLACZNYCH nazwach moga wskazywac jeden magazyn. Slot niesie sam skrot, wiec zwracana jest
///       sciezka wolajacego.
[[nodiscard]] std::optional<StoreOwner> findForeignStoreOwner(const std::vector<InstanceInfo> &instances,
                                                              std::string_view selfName, const std::vector<std::string> &stores);

enum class ClaimStatus : std::uint8_t {
  Claimed,          ///< slot zajety, nazwy strumieni rozlaczne ze wszystkimi zywymi instancjami
  Conflict,         ///< nazwa strumienia nalezy juz do zywej instancji
  CounterConflict,  ///< plik licznika :ROTATION jest juz uzywany przez zywa instancje
  StoreConflict,    ///< plik magazynu strumienia jest juz zapisywany przez zywa instancje
  ServiceConflict,  ///< zywa instancja pracuje juz w trybie uslugowym; usluga jest dokladnie jedna
  TooLarge,         ///< plan przekracza pojemnosc slotu
  NoFreeSlot,       ///< wszystkie sloty zajete przez zywe instancje
  Unavailable,      ///< magistrali nie da sie uzyc; start bez egzekwowania rozlacznosci
  NotReserved,      ///< tylko activateReservedPlan(): przylaczona instancja bez slotu albo bez rezerwacji
};

/// @requirements-exempt ClaimResult: Wynik rezerwacji zasobow; decyzje podejmuje magistrala.
struct ClaimResult {
  ClaimStatus status{ClaimStatus::Unavailable};
  std::string stream;     ///< kolidujaca nazwa strumienia (Conflict)
  std::string ownerName;  ///< wlasciciel kolidujacego zasobu; pusty => bezimienny
  std::int32_t ownerPid{0};
  std::string detail;  ///< sciezka licznika (CounterConflict) albo magazynu (StoreConflict), powod niedostepnosci albo limit
  /// Wylacznie claimAdditional() przy Claimed: nazwy i sciezki faktycznie dopisane do slotu, bez tych,
  /// ktore juz w nim staly. To one, i tylko one, wracaja do releaseAdditional() przy wycofaniu.
  std::vector<std::string> addedStreams;
  std::vector<std::string> addedStores;
};

/// Komplet danych, ktore instancja publikuje w swoim slocie.
/// @requirements-exempt ClaimRequest: Parametry rezerwacji zasobow publikowane przez instancje.
struct ClaimRequest {
  std::string_view name;         ///< pusta => instancja historyczna (bez --name)
  std::string_view queryFile;    ///< plik zapytan, z ktorego instancja wystartowala
  std::string_view unit;         ///< nazwa jednostki systemd; pusta => zwykly proces
  std::string_view counterPath;  ///< znormalizowana sciezka licznika :ROTATION; pusta => brak rotacji
  std::uint32_t modes{0};        ///< maska bus::mode::*; 0 => tryb zwykly
  std::vector<std::string> streams;
  /// Znormalizowane sciezki plikow magazynu; puste => plan nic nie zapisuje. Skrot z nich robi
  /// dopiero magistrala: wolajacy operuje sciezkami, bo to one wracaja w komunikacie o kolizji.
  std::vector<std::string> stores;
};

/// Funkcja processStartTime powinna:
/// - [bus.process-start-time.1~1] zwracać znacznik startu procesu z osplat::inspectProcess (Linux: pole 22 starttime ze stat lidera grupy wątków, Darwin: czas z sysctl), a 0, gdy procesu nie ma albo jego wpisu nie da się odczytać.
///   Zakres testow: `BusFixture.ProcessIdentityAgreesAcrossProcesses`, `BusFixture.ProcessStateDecidesLiveness`; na Linuksie biegnie galaz procfs, galaz sysctl (Darwin) tylko w buildzie macOS.
[[nodiscard]] std::uint64_t processStartTime(std::int32_t pid);

/// Funkcja isProcessAlive powinna:
/// - [bus.is-process-alive.1~1] dla pid <= 0 zwracać false.
/// - [bus.is-process-alive.2~1] dla wpisu procesu, którego nie da się odczytać (hidepid, ProtectProc), logować ostrzeżenie i zwracać true.
///   Zakres testow: `BusFixture.UnreadableOwnerKeepsSlot`; ostrzezenie (SPDLOG_WARN) bez asercji - Release kompiluje je bez wyjscia.
/// - [bus.is-process-alive.3~1] dla procesu nieistniejącego i dla procesu zombie zwracać false.
/// - [bus.is-process-alive.4~1] w pozostałych przypadkach zwracać true wyłącznie przy niezerowym znaczniku startu równym startTime, także dla procesu zatrzymanego sygnałem albo w nieprzerywalnym śnie.
///   Zakres testow: `BusFixture.LivenessRequiresMatchingStartTime`, `BusFixture.ProcessStateDecidesLiveness`; nieprzerywalny sen i czytelny wpis z zerowym znacznikiem bez przypadku - nie da sie ich wywolac deterministycznie.
///
/// @note Zgodnosc startTime odroznia ten sam proces od innego, ktoremu jadro nadalo zwolniony PID. Zombie
///       trzymalby nazwy strumieni az do wait() rodzica. Wpis nieczytelny to "nie wiem": falszywe "zyje"
///       kosztuje zajety slot, a falszywe "martwy" - dwie instancje na jednym <qryID>.desc.
[[nodiscard]] bool isProcessAlive(std::int32_t pid, std::uint64_t startTime);

/// Obiekt klasy Bus powinien:
/// - [bus.bus.1~1] przy konstrukcji zajmować współdzieloną blokadę pliku obecności przed otwarciem segmentu i podłączać się do segmentu o podanej nazwie, tworząc go wyłącznie w trybie createIfMissing, a przy niepowodzeniu przechodzić w stan niepodłączony bez przerywania startu serwera.
/// - [bus.bus.2~1] raportować przez attached, czy segment jest odwzorowany.
/// - [bus.bus.17~1] przy podłączeniu odrzucać niezgodny wariant lub rozmiar BusMutex zapisany w wersji układu, zanim odczyta muteks lub sloty.
/// - [bus.bus.18~1] w gałęzi atomowej po ponad 10 sekundach oczekiwania na zamek żywego właściciela wypisywać diagnostykę ERROR dostępną także w Release, bez przejmowania jego zamka.
/// - [bus.bus.3~1] w claim po kontroli limitów zwalniać poprzedni slot tej instancji, a następnie pod muteksem magistrali zajmować wolny lub martwy slot wyłącznie wtedy, gdy nazwy strumieni, ścieżka licznika :ROTATION i skróty ścieżek magazynu są rozłączne ze wszystkimi żywymi instancjami i żadna żywa instancja nie pracuje w trybie usługowym, gdy żąda go ta instancja; przy TooLarge i przy niepodłączonej magistrali zachowywać poprzedni slot.
///   Zakres testow: `BusFixture.ReclaimReplacesOwnSlot`, `BusFixture.ReclaimRefusedByLimitsKeepsPreviousSlotButConflictDropsIt`, `BusFixture.ZombieSlotIsFreeAgain`, `BusFixture.SecondServiceInstanceIsRefused`, `FallbackLock.LiveStoppedOwnerKeepsLockBeyondDiagnosticDeadline`; magistrala niepodlaczona nigdy nie ma slotu, wiec ta polowa jest pusta.
/// - [bus.bus.4~1] zwracać z claim odpowiednio Conflict, CounterConflict, StoreConflict albo ServiceConflict z kolidującym zasobem i jego właścicielem, TooLarge przy przekroczeniu pojemności slotu lub długości pól rozstrzygających, NoFreeSlot przy braku wolnego slotu i Unavailable, gdy magistrali nie da się użyć.
///   Zakres testow: `BusFixture.CollidingStreamIsRefusedWithOwner`, `BusFixture.SharedRotationCounterIsRefused`, `BusFixture.SharedStorageFileIsRefused`, `BusFixture.SecondServiceInstanceIsRefused`, `BusFixture.OversizedPlanIsRefused`, `BusFixture.OversizedCounterPathIsRefused`, `BusFixture.FullBusIsNoFreeSlot`, `BusFixture.ReaderModeNeverCreatesSegmentAndIsUnavailable`, `BusRepairSeqlock.UnrecoverableMutexReportsRecoveryInstructions` (ut_busSlotCount, galaz robust).
/// - [bus.bus.5~1] obcinać zbyt długie pola informacyjne (nazwa jednostki, plik zapytań) z ostrzeżeniem w logu zamiast odmawiać startu.
///   Zakres testow: `BusFixture.InformationalFieldsAreTruncatedNotRefused`; ostrzezenie (SPDLOG_WARN) bez asercji - Release kompiluje je bez wyjscia.
/// - [bus.bus.6~1] uznawać slot za żywy wyłącznie, gdy proces o jego pid istnieje, nie jest zombie i ma ten sam czas startu, a slot martwy traktować jako wolny bez demona i heartbeatów.
///   Zakres testow: `BusFixture.ZombieSlotIsFreeAgain`, `BusFixture.UnreadableOwnerKeepsSlot`, `FallbackLock.SlotWithForeignStartTimeIsFree` (ut_busFallback; kod zywotnosci slotu wspolny dla obu galezi zamka).
/// - [bus.bus.7~1] w reservePlan rezerwować w posiadanym slocie zasoby następnego planu po kontroli rozłączności pod muteksem, pozostawiając aktywne zasoby publikowane, a przy odmowie nie zmieniać slotu; bez posiadanego slotu zwracać Unavailable.
/// - [bus.bus.8~1] w activateReservedPlan zastępować aktywne zasoby zarezerwowanymi bez ponownej kontroli kolizji, zwracając NotReserved dla instancji bez slotu lub rezerwacji i Unavailable, gdy magistrali nie da się użyć.
/// - [bus.bus.9~1] w claimAdditional dopisywać do posiadanego slotu tylko nazwy i ścieżki nieobecne w nim, po kontroli rozłączności, zwracać faktycznie dopisane w addedStreams i addedStores, a przy odmowie nie zmieniać slotu.
/// - [bus.bus.10~1] w releaseAdditional zdejmować z własnego slotu wyłącznie podane nazwy i ścieżki, bez zmiany reszty slotu, a bez slotu nie robić nic.
/// - [bus.bus.11~1] w release zwalniać własny slot pod muteksem, idempotentnie, także z obsługi atexit.
///   Zakres testow: `BusFixture.ReleaseIsIdempotent`, `BusFixture.ReleaseFreesTheName`; wywolanie z obslugi atexit (executorsm.cpp) bez przypadku jednostkowego, sprzatanie po wyjsciu sprawdza `it_leftover_sweep`.
/// - [bus.bus.12~1] przez instances zwracać migawkę żywych instancji czytaną seqlockiem, bez muteksu i bez kontaktu z serwerami, pomijając sloty w trakcie zapisu i sloty martwe.
///   Zakres testow: `BusFixture.ClaimPublishesInstance`, `BusFixture.ZombieSlotIsFreeAgain`, `FallbackLock.InstancesSkipSlotMidWriteWithoutTakingLock` (ut_busFallback; odczyt wspolny dla obu galezi zamka).
/// - [bus.bus.13~1] po przejęciu muteksu porzuconego przez zmarłego właściciela unieważniać sloty przerwane w trakcie zapisu przed oznaczeniem stanu jako spójnego.
///   Zakres testow: `BusRepairSeqlock.RepairPublishesFreshEvenSequence` (ut_busSlotCount i ut_busFallback), `BusRepairSeqlock.InterruptedRepairRemainsRecoverable` (ut_busSlotCount), `FallbackLock.DeadOwnerIsRecoveredAndInterruptedSlotInvalidated`, `BusRepairSeqlock.InterruptedSlotIsRepairedBeforeMutexIsMarkedConsistent` (ut_busSlotCount, przechwycenie pthread_mutex_consistent bez haka w produkcji).
/// - [bus.bus.14~1] przy zniszczeniu zwalniać slot i odwzorowanie, a segment i plik obecności usuwać wyłącznie wtedy, gdy uda się zająć wyłączną blokadę obecności, czyli segmentu nie mapuje nikt inny.
/// - [bus.bus.15~1] uniemożliwiać kopiowanie obiektu.
/// - [bus.bus.16~1] gdy w releaseAdditional muteksu magistrali nie da się użyć, logować błąd z nazwami strumieni i ścieżkami magazynów, które zostają zajęte, zamiast wracać bez śladu.
///   Sprawdzenie kompilacji: test/UnitTest/test_bus.cpp - static_assert(!std::is_copy_constructible_v<bus::Bus> && !std::is_copy_assignable_v<bus::Bus>).
class Bus {
 public:
  /// Podlacza sie do segmentu, tworzac go, gdy jeszcze nie istnieje. Niepowodzenie nie jest
  /// bledem startu serwera -- obiekt przechodzi wtedy w stan niepodlaczony, a claim() zwraca
  /// Unavailable.
  ///
  /// Nazwa segmentu jest parametrem OBOWIAZKOWYM. Wartosc domyslna kSegmentName byla tu
  /// pulapka od chwili wprowadzenia przestrzeni nazw uruchomienia: wolanie bez argumentu
  /// omijalo ja po cichu i sadzalo instancje na segmencie produkcyjnym. Program uzywa
  /// segmentName(), test jednostkowy swojego wlasnego segmentu.
  ///
  /// createIfMissing = false to tryb CZYTELNIKA (xqry). Klient nie ma prawa zakladac
  /// magistrali: pusty segment nie niesie zadnej informacji, a jego brak znaczy dokladnie
  /// tyle, ze zaden serwer jeszcze nie wystartowal -- czyli stan normalny, nie awaria.
  explicit Bus(std::string_view segmentName, bool createIfMissing = true);
  ~Bus();

  Bus(const Bus &)            = delete;
  Bus &operator=(const Bus &) = delete;

  [[nodiscard]] bool attached() const;

  /// Sprawdza rozlacznosc nazw strumieni, sciezki licznika :ROTATION ORAZ sciezek plikow
  /// magazynu ze wszystkimi zywymi instancjami i -- gdy sa rozlaczne -- zatwierdza wlasny slot.
  /// Roszczenia, rezerwacje, aktywacje i zwolnienia sa serializowane jednym muteksem magistrali.
  ///
  /// Licznik jest chroniony osobno, bo nie jest nazwa strumienia: PersistentCounter czyta
  /// wartosc i zapisuje nastepna bez blokady pliku, wiec dwie instancje na jednym pliku moga
  /// dostac ten sam numer i nadpisac sobie archiwa. Sciezke normalizuje WOLAJACY -- magistrala
  /// porownuje napisy, a nie pliki.
  ///
  /// Magazyn jest chroniony osobno z tego samego powodu, choc wyglada na pochodna nazwy: klauzula
  /// `FILE` odrywa nazwe pliku od nazwy zapytania (streamInstance bierze wtedy qry.filename), wiec
  /// dwa plany o rozlacznych nazwach strumieni moga pisac do JEDNEGO pliku. Nazwa strumienia broni
  /// wylacznie deskryptora `<qryID>.desc`, bo tylko on jest budowany z identyfikatora.
  ///
  /// Tu tez zapada rozstrzygniecie "usluga jest dokladnie jedna" (mode::kService). Kontrola musi
  /// byc TUTAJ, a nie u wolajacego przed roszczeniem: instances() czyta seqlockiem, bez muteksu,
  /// wiec instancja miedzy sprawdzeniem a zapisem swojego slotu jest dla drugiej niewidoczna
  /// i dwa rownolegle starty z roznymi nazwami przechodzily oba. claim() jest jedynym miejscem,
  /// w ktorym maska trybow trafia do slotu, wiec pod jego muteksem sprawdzenie jest atomowe.
  ClaimResult claim(const ClaimRequest &request);

  /// Rezerwuje nazwy strumieni, licznik rotacji i pliki magazynu dla nastepnego planu w JUZ posiadanym
  /// slocie. Limity i kolizje sa rozstrzygane pod muteksem magistrali, zanim stary plan
  /// zostanie rozebrany. Przy odmowie slot pozostaje nietkniety; przy sukcesie aktywne
  /// zasoby nadal sa publikowane przez instances(), a rezerwacja blokuje obce roszczenia.
  ///
  /// Wymaga posiadanego slotu (po udanym claim()); bez niego zwraca Unavailable.
  ClaimResult reservePlan(const std::vector<std::string> &streams, std::string_view counterPath,
                          const std::vector<std::string> &stores);

  /// Zastepuje aktywne zasoby uprzednio zarezerwowanymi. Operacja nie moze juz wejsc
  /// w kolizje: od udanego reservePlan() rezerwacja uczestniczy we wszystkich roszczeniach.
  /// Dwie porazki sa rozne z natury, wiec maja rozne statusy: Unavailable - magistrali nie da
  /// sie uzyc (zamek, srodowisko), NotReserved - przylaczona instancja nie ma slotu albo
  /// rezerwacji, czyli zlamana kolejnosc wywolan po stronie silnika.
  ClaimResult activateReservedPlan();

  /// Dopisuje nazwy strumieni i sciezki ich magazynow do JUZ posiadanego slotu, sprawdziwszy
  /// ich rozlacznosc z pozostalymi zywymi instancjami. Sluzy zapytaniom ad-hoc, ktore
  /// powiekszaja plan dzialajacego serwera o nowe nazwy.
  ///
  /// To NIE jest claim() wolane powtornie: claim() zaczyna od release(), wiec odmowa
  /// zostawialaby dzialajacy serwer bez slotu, czyli takze bez roszczenia nazw, ktore
  /// juz obsluguje. Tutaj odmowa nie ma zadnego skutku ubocznego -- slot zostaje
  /// nietkniety. Nazwy i sciezki juz obecne w slocie sa pomijane, wiec operacja jest idempotentna.
  /// Co faktycznie dopisano, wraca w ClaimResult::addedStreams i ClaimResult::addedStores.
  ///
  /// Wymaga posiadanego slotu (po udanym claim()); bez niego zwraca Unavailable.
  ClaimResult claimAdditional(const std::vector<std::string> &streams, const std::vector<std::string> &stores);

  /// Zdejmuje z WLASNEGO slotu podane nazwy strumieni i sciezki magazynow -- odwrotnosc udanego
  /// claimAdditional() dla zapytania ad-hoc, ktorego import do planu sie nie powiodl. Bez tego
  /// nazwa, ktorej plan nie zawiera, bylaby ogloszona jako strumien tej instancji az do jej konca.
  ///
  /// Wolajacy podaje WYLACZNIE to, co wrocilo w addedStreams/addedStores: zdjecie calej listy
  /// z zapytania zabraloby nazwy, ktore plan juz obsluguje. Reszta slotu -- pozostale nazwy,
  /// rezerwacja planu, licznik i tryby -- zostaje nietknieta. Bez slotu operacja nic nie robi.
  void releaseAdditional(const std::vector<std::string> &streams, const std::vector<std::string> &stores);

  /// Zwalnia slot tej instancji. Idempotentne; wolane takze z handlera atexit.
  void release();

  /// Migawka zywych instancji. Czysty odczyt seqlockiem, bez muteksu i bez kontaktu
  /// z serwerami.
  [[nodiscard]] std::vector<InstanceInfo> instances() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};

}  // namespace bus
