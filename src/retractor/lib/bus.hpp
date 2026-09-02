#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
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
/// zauwazy -- bez demona i bez heartbeatow.
namespace bus {

/// Nazwa segmentu w /dev/shm. Segment nie jest przez nikogo kasowany: usuniecie go w chwili,
/// gdy inna instancja trzyma odwzorowanie, zerwaloby jej magistrale.
inline constexpr std::string_view kSegmentName = "xrdbbus";

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
/// (test/IntegrationTest_serial/optimizer_ablation) ma 53 wezly.
inline constexpr std::size_t kMaxSlots         = 32;
inline constexpr std::size_t kMaxStreams       = 128;
inline constexpr std::size_t kStreamNameSize   = 208;  ///< z terminatorem => nazwa do 207 znakow
inline constexpr std::size_t kInstanceNameSize = 40;   ///< servername::kMaxLength (32) + zapas
inline constexpr std::size_t kQueryFileSize    = 256;  ///< z terminatorem => sciezka do 255 znakow

/// Opis jednej zywej instancji odczytany z magistrali.
struct InstanceInfo {
  std::string name;  ///< pusta => instancja historyczna (bez --name)
  std::int32_t pid{0};
  std::string queryFile;
  std::vector<std::string> streams;
};

enum class ClaimStatus : std::uint8_t {
  Claimed,      ///< slot zajety, nazwy strumieni rozlaczne ze wszystkimi zywymi instancjami
  Conflict,     ///< nazwa strumienia nalezy juz do zywej instancji
  TooLarge,     ///< plan przekracza pojemnosc slotu
  NoFreeSlot,   ///< wszystkie sloty zajete przez zywe instancje
  Unavailable,  ///< magistrali nie da sie uzyc; start bez egzekwowania rozlacznosci
};

struct ClaimResult {
  ClaimStatus status{ClaimStatus::Unavailable};
  std::string stream;     ///< kolidujaca nazwa strumienia (Conflict)
  std::string ownerName;  ///< wlasciciel kolidujacej nazwy (Conflict); pusty => bezimienny
  std::int32_t ownerPid{0};
  std::string detail;  ///< powod niedostepnosci albo przekroczony limit -- do logu
};

/// Czas startu procesu: pole 22 z /proc/<pid>/stat. Zwraca 0, gdy procesu nie ma
/// albo pliku nie da sie odczytac.
[[nodiscard]] std::uint64_t processStartTime(std::int32_t pid);

/// Czy proces o podanym PID nadal zyje. Zgodnosc startTime odrozania ten sam proces
/// od innego procesu, ktoremu jadro nadalo ten sam, zwolniony wczesniej PID.
[[nodiscard]] bool isProcessAlive(std::int32_t pid, std::uint64_t startTime);

class Bus {
 public:
  /// Podlacza sie do segmentu, tworzac go, gdy jeszcze nie istnieje. Niepowodzenie nie jest
  /// bledem startu serwera -- obiekt przechodzi wtedy w stan niepodlaczony, a claim() zwraca
  /// Unavailable. Nazwa segmentu jest parametrem wylacznie po to, by test jednostkowy nie
  /// dotykal magistrali dzialajacych serwerow.
  explicit Bus(std::string_view segmentName = kSegmentName);
  ~Bus();

  Bus(const Bus &)            = delete;
  Bus &operator=(const Bus &) = delete;

  [[nodiscard]] bool attached() const;

  /// Sprawdza rozlacznosc nazw strumieni ze wszystkimi zywymi instancjami i -- gdy sa
  /// rozlaczne -- zatwierdza wlasny slot. Jedyna operacja wymagajaca serializacji.
  ClaimResult claim(std::string_view serverName, std::string_view queryFile, const std::vector<std::string> &streams);

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
