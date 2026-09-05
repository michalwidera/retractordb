#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/// @brief Magistrala xrdbbus: wspolny obszar wykrywania instancji xretractor i egzekwowania
///        rozlacznosci nazw strumieni miedzy serwerami na jednej maszynie.
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

/// Nazwa segmentu w /dev/shm. Segment nie jest przez nikogo kasowany: usuniecie go w chwili,
/// gdy inna instancja trzyma odwzorowanie, zerwaloby jej magistrale.
///
/// NAZWA NIESIE WERSJE UKLADU i przy KAZDEJ zmianie ukladu slotu musi isc w gore razem
/// z kLayoutVersion. Powod jest wdrozeniowy: segment o starym ukladzie zostaje w /dev/shm po
/// podmianie binarki, a instancja, ktora odmowi sie do niego podlaczyc, startuje BEZ egzekwowania
/// rozlacznosci nazw -- czyli awaria jest cicha az do pierwszej kolizji. Z wersja w nazwie nowa
/// binarka po prostu zaklada wlasny segment, a stary zostaje nieuzywanym smieciem do restartu
/// maszyny. Automatycznego kasowania swiadomie nie ma: "sprawdz, czy nikt nie zyje, i skasuj" jest
/// wyscigiem, w ktorym dwie instancje moga skonczyc na DWOCH segmentach, kazda widzac tylko siebie.
///
/// Podkreslenie, nie kropka: obiekty IPC instancji nazywaja sie "<obiekt>.<nazwa instancji>", wiec
/// "xrdbbus.v2" wygladalby jak obiekt instancji o nazwie "v2" i wpadl pod wzorce sprzatajace
/// postaci /dev/shm/*.<nazwa>.
inline constexpr std::string_view kSegmentName = "xrdbbus_v3";

/// Nazwa segmentu dla BIEZACEGO uruchomienia: kSegmentName, a przy ustawionej przestrzeni
/// nazw (servername::environmentNamespace) kSegmentName + "_" + przestrzen.
///
/// Rozlacznosc nazw strumieni jest wlasnoscia CALEJ maszyny, wiec dwa niezalezne zestawy
/// testow integracyjnych uzywajace tych samych nazw (`core0`, `dst`, `str1` powtarzaja sie
/// w kilkunastu katalogach) nie moga biec obok siebie na jednym segmencie -- drugi start
/// odpada na ClaimStatus::Conflict. Wlasny segment daje kazdemu z nich wlasna przestrzen
/// nazw strumieni, bez ruszania samych zapytan i plikow wzorcowych.
///
/// Podkreslenie, tak samo jak przy czlonie wersji: kropka jest zarezerwowana dla obiektow
/// instancji ("<obiekt>.<nazwa instancji>") i wpadalaby pod wzorce sprzatajace.
[[nodiscard]] std::string segmentName();

/// Pojemnosci ukladu. Przekroczenie ktoregokolwiek limitu jest bledem startu, a nie cichym
/// zawieszeniem gwarancji rozlacznosci -- slot z obcieta lista strumieni nie moglby juz
/// odpowiadac na pytanie "czyja jest ta nazwa".
///
/// Dlugosc nazwy strumienia jest zwiazana z budzetem kompilatora: substratNameBudget_C
/// (compiler.cpp) wynosi 200 znakow, a nazwy dluzsze compiler::composeStreamName zastepuje
/// skrotem. Pole krotsze niz ten budzet wywracalo start planow z szerokim FROM -- nazwa
/// STREAM_ADD_STREAM_ADD_..._str01_..._str12 z it_wide_from_names ma ponad 130 znakow.
///
/// Liczba strumieni ma zapas rzedu 2,5x: najwiekszy skompilowany plan w repozytorium
/// (test/IntegrationTest/optimizer_ablation) ma 53 wezly.
inline constexpr std::size_t kMaxSlots         = 32;
inline constexpr std::size_t kMaxStreams       = 128;
inline constexpr std::size_t kStreamNameSize   = 208;  ///< z terminatorem => nazwa do 207 znakow
inline constexpr std::size_t kInstanceNameSize = 40;   ///< servername::kMaxLength (32) + zapas
inline constexpr std::size_t kQueryFileSize    = 256;  ///< z terminatorem => sciezka do 255 znakow
inline constexpr std::size_t kUnitNameSize     = 128;  ///< nazwa jednostki systemd; limit systemd to 256, tu z zapasem
inline constexpr std::size_t kCounterPathSize  = 256;  ///< znormalizowana sciezka licznika :ROTATION

/// Przekroczenie limitu ma DWA rozne skutki, i podzial nie jest dowolny.
///
/// Pola ROZSTRZYGAJACE rozlacznosc -- nazwa strumienia i sciezka licznika -- to odmowa startu:
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

/// Opis jednej zywej instancji odczytany z magistrali.
struct InstanceInfo {
  std::string name;  ///< pusta => instancja historyczna (bez --name)
  std::int32_t pid{0};
  std::string queryFile;
  std::string unit;         ///< nazwa jednostki systemd; pusta => zwykly proces
  std::string counterPath;  ///< sciezka licznika :ROTATION; pusta => plan bez rotacji
  std::uint32_t modes{0};   ///< maska bus::mode::*; 0 => tryb zwykly
  std::vector<std::string> streams;
};

/// Wlasciciel nazwy strumienia znaleziony w migawce magistrali.
struct StreamOwner {
  std::string stream;    ///< kolidujaca nazwa
  std::string instance;  ///< wlasciciel; pusta => instancja bezimienna
  std::int32_t pid{0};
};

/// Wlasciciel pliku licznika rotacji znaleziony w migawce magistrali.
struct CounterOwner {
  std::string path;
  std::string instance;
  std::int32_t pid{0};
};

/// Szuka w migawce nazwy z `streams`, ktora nalezy do instancji INNEJ niz `selfName`.
///
/// Sluzy sprawdzeniu zestawu zapytan PRZED dostarczeniem go dzialajacemu serwisowi: instancje
/// docelowa trzeba pominac, bo restart i tak zwalnia jej slot, a kolizja z nia sama nie jest
/// kolizja. Czysta funkcja nad gotowa migawka -- bez IPC i bez kontaktu z serwerami.
[[nodiscard]] std::optional<StreamOwner> findForeignOwner(const std::vector<InstanceInfo> &instances, std::string_view selfName,
                                                          const std::vector<std::string> &streams);

/// Odpowiednik findForeignOwner dla znormalizowanej sciezki licznika :ROTATION. Pusta
/// sciezka nie rości zasobu. Instancja docelowa jest pomijana, bo dostarczenie planu
/// zastępuje jej dotychczasowy slot po restarcie.
[[nodiscard]] std::optional<CounterOwner> findForeignCounterOwner(const std::vector<InstanceInfo> &instances,
                                                                  std::string_view selfName, std::string_view counterPath);

enum class ClaimStatus : std::uint8_t {
  Claimed,          ///< slot zajety, nazwy strumieni rozlaczne ze wszystkimi zywymi instancjami
  Conflict,         ///< nazwa strumienia nalezy juz do zywej instancji
  CounterConflict,  ///< plik licznika :ROTATION jest juz uzywany przez zywa instancje
  TooLarge,         ///< plan przekracza pojemnosc slotu
  NoFreeSlot,       ///< wszystkie sloty zajete przez zywe instancje
  Unavailable,      ///< magistrali nie da sie uzyc; start bez egzekwowania rozlacznosci
};

struct ClaimResult {
  ClaimStatus status{ClaimStatus::Unavailable};
  std::string stream;     ///< kolidujaca nazwa strumienia (Conflict)
  std::string ownerName;  ///< wlasciciel kolidujacego zasobu; pusty => bezimienny
  std::int32_t ownerPid{0};
  std::string detail;  ///< sciezka licznika (CounterConflict), powod niedostepnosci albo limit
};

/// Komplet danych, ktore instancja publikuje w swoim slocie.
struct ClaimRequest {
  std::string_view name;         ///< pusta => instancja historyczna (bez --name)
  std::string_view queryFile;    ///< plik zapytan, z ktorego instancja wystartowala
  std::string_view unit;         ///< nazwa jednostki systemd; pusta => zwykly proces
  std::string_view counterPath;  ///< znormalizowana sciezka licznika :ROTATION; pusta => brak rotacji
  std::uint32_t modes{0};        ///< maska bus::mode::*; 0 => tryb zwykly
  std::vector<std::string> streams;
};

/// Czas startu procesu: pole 22 z /proc/<pid>/stat. Zwraca 0, gdy procesu nie ma
/// albo pliku nie da sie odczytac.
[[nodiscard]] std::uint64_t processStartTime(std::int32_t pid);

/// Czy proces o podanym PID nadal zyje. Zgodnosc startTime odrozania ten sam proces
/// od innego procesu, ktoremu jadro nadalo ten sam, zwolniony wczesniej PID.
///
/// Proces ZOMBIE (stan 'Z') jest martwy: niezebrany potomek zachowuje /proc/<pid>/stat
/// razem ze starttime, wiec bez sprawdzenia stanu trzymalby swoje nazwy strumieni az do
/// wait() rodzica. Stan czytany jest z tej samej linii co starttime, czyli za darmo.
/// Odrzucany jest wylacznie 'Z' -- uzasadnienie w bus.cpp przy definicji.
[[nodiscard]] bool isProcessAlive(std::int32_t pid, std::uint64_t startTime);

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

  /// Sprawdza rozlacznosc nazw strumieni ORAZ sciezki licznika :ROTATION ze wszystkimi zywymi
  /// instancjami i -- gdy sa rozlaczne -- zatwierdza wlasny slot. Jedyna operacja wymagajaca
  /// serializacji.
  ///
  /// Licznik jest chroniony osobno, bo nie jest nazwa strumienia: PersistentCounter wczytuje
  /// wartosc przy starcie, a zapisuje ja dopiero w destruktorze, wiec dwie instancje na jednym
  /// pliku zapisuja te sama wartosc i gubia rotacje. Sciezke normalizuje WOLAJACY -- magistrala
  /// porownuje napisy, a nie pliki.
  ClaimResult claim(const ClaimRequest &request);

  /// Dopisuje nazwy strumieni do JUZ posiadanego slotu, sprawdziwszy ich rozlacznosc
  /// z pozostalymi zywymi instancjami. Sluzy zapytaniom ad-hoc, ktore powiekszaja plan
  /// dzialajacego serwera o nowe nazwy.
  ///
  /// To NIE jest claim() wolane powtornie: claim() zaczyna od release(), wiec odmowa
  /// zostawialaby dzialajacy serwer bez slotu, czyli takze bez roszczenia nazw, ktore
  /// juz obsluguje. Tutaj odmowa nie ma zadnego skutku ubocznego -- slot zostaje
  /// nietkniety. Nazwy juz obecne w slocie sa pomijane, wiec operacja jest idempotentna.
  ///
  /// Wymaga posiadanego slotu (po udanym claim()); bez niego zwraca Unavailable.
  ClaimResult claimAdditional(const std::vector<std::string> &streams);

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
