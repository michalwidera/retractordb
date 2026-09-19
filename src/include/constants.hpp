#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

#include "platformConfig.h"

namespace constants {
constexpr std::string_view Reserved_id_oob = "OUT_OF_BUSSINESS";

/// Odpowiedz serwera na komende wymagajaca modelu danych, gdy instancja nie ma wczytanego
/// planu (tryb bezczynny). Wspolna dla obu stron IPC: serwer ja wpisuje, klient rozpoznaje.
/// Bez wspolnej stalej klient nie odroznial instancji bezczynnej od serwera, ktory nie
/// odpowiedzial - i meldowal timeout tam, gdzie odpowiedz przyszla od razu.
constexpr std::string_view kNoActivePlanReply = "no active plan";

/// Odpowiedz serwera, ktory przyjal komende juz w trakcie wlasnego zamykania. Wspolna dla
/// obu stron IPC z tego samego powodu co kNoActivePlanReply: bez niej klient wrzucal ten
/// stan do worka "serwer nie odpowiedzial" i meldowal timeout tam, gdzie odpowiedz przyszla
/// od razu i byla prawdziwa.
constexpr std::string_view kServerStoppingReply = "server stopping";
}  // namespace constants

namespace ipc {

// === Shared memory / mutex / queue names ===
// Muszą być spójne między serwerem (executorsm) a klientem (ipcClient, qryLauncher).

// Segment shared memory przechowujący mapę odpowiedzi per-PID.
constexpr std::string_view kShmemSegment = "RetractorShmemMap";

// Named mutex chroniący dostęp do mapy w shared memory.
constexpr std::string_view kMapMutex = "RetractorMapMutex";

// Główna kolejka komend: klient wysyła, serwer odbiera.
constexpr std::string_view kQueryQueue = "RetractorQueryQueue";

// Nazwa obiektu mapy wewnątrz segmentu shared memory.
constexpr std::string_view kMapObject = "MyMap";

// Prefiks nazwy kolejki odpowiedzi per-proces; pełna nazwa = prefiks + PID.
constexpr std::string_view kResponseQueuePrefix = "brcdbr";

// === Nazwy obiektów IPC jednego serwera ===
//
// Powyższe stałe są nazwami BAZOWYMI. Komplet obiektów jednego serwera wyróżnia nazwa
// serwera doklejana jako sufiks, dzięki czemu obszary IPC kolejnych serwerów są rozłączne
// i żaden z nich nie może skasować cudzych obiektów.
//
// Pusta nazwa serwera daje dokładnie nazwy historyczne (jednoserwerowe). To jest celowe:
// sama parametryzacja niczego nie zmienia w zachowaniu, a rozdział obszarów włącza się
// dopiero wtedy, gdy ktoś poda nazwę niepustą.
struct ServerNames {
  std::string shmemSegment;
  std::string mapMutex;
  std::string queryQueue;
  std::string responseQueuePrefix;

  /// Nazwa kolejki odpowiedzi konkretnego klienta.
  [[nodiscard]] std::string responseQueue(int clientId) const { return responseQueuePrefix + std::to_string(clientId); }
};

/// Najdluzsza nazwa obiektu IPC, ktora jadro przyjmie.
///
/// Na jadrach BSD-owych nazwy POSIX-owych semaforow i obiektow pamieci dzielonej
/// sa ograniczone do PSEMNAMLEN / PSHMNAMLEN, czyli 31 znakow, a dluzsza konczy
/// sie ENAMETOOLONG ("File name too long") juz przy TWORZENIU obiektu. Dotyczy to
/// takze obiektow, ktorych sami nie zakladamy: Boost.Interprocess realizuje tam
/// named_mutex przez sem_open, bo Darwin nie ma muteksow wspoldzielonych miedzy
/// procesami. Na Linuksie limitem jest NAME_MAX (255) i zapas jest tak duzy, ze
/// warunek ponizej nigdy nie zadziala - nazwy zostaja doslownie takie jak dotad.
inline constexpr std::size_t kMaxObjectNameLength = RDB_OS_DARWIN ? 31 : 200;

/// Zapas na czlon doklejany PO zlozeniu nazwy bazowej: identyfikator klienta
/// w nazwie kolejki odpowiedzi plus ukosnik, ktory Boost stawia z przodu.
inline constexpr std::size_t kObjectNameTailBudget = 12;

/// Skrot nazwy serwera: osiem cyfr szesnastkowych FNV-1a.
///
/// Liczony JAWNIE, a nie przez std::hash, z tego samego powodu co skrot sciezki
/// magazynu w bus.hpp: wartosc musi byc identyczna po obu stronach IPC, a wynik
/// std::hash jest szczegolem implementacji biblioteki standardowej.
inline std::string shortServerTag(std::string_view serverName) {
  std::uint64_t hash = 0xcbf2'9ce4'8422'2325ULL;
  for (const unsigned char byte : serverName) {
    hash ^= byte;
    hash *= 0x0000'0100'0000'01B3ULL;
  }
  // NOLINTNEXTLINE(modernize-avoid-c-arrays): bufor dla snprintf
  char buffer[9];
  std::snprintf(buffer, sizeof(buffer), "%08x", static_cast<unsigned>(hash ^ (hash >> 32)));
  return {buffer};
}

/// Czlon nazwy obiektu odpowiadajacy tej instancji: sama nazwa serwera, a gdy
/// komplet nazw nie miescilby sie w limicie platformy - jej skrot.
///
/// Decyzje podejmuje NAJDLUZSZA z nazw (kolejka odpowiedzi z identyfikatorem
/// klienta), zeby wszystkie obiekty jednej instancji byly nazwane jednakowo:
/// serwer i klient licza to niezaleznie i musza dojsc do tej samej nazwy.
inline std::string serverNameToken(std::string_view serverName) {
  if (serverName.empty()) return {};
  const std::size_t longest = kResponseQueuePrefix.size() + 1 + serverName.size() + 1 + kObjectNameTailBudget;
  if (longest <= kMaxObjectNameLength) return std::string(serverName);
  return shortServerTag(serverName);
}

/// Nazwa bazowa z sufiksem serwera; bez sufiksu, gdy nazwa serwera pusta.
inline std::string withServerSuffix(std::string_view base, std::string_view serverName) {
  std::string retVal(base);
  if (!serverName.empty()) {
    retVal += '.';
    retVal += serverName;
  }
  return retVal;
}

inline ServerNames names(std::string_view serverName = {}) {
  ServerNames retVal;
  // Czlon instancji liczony RAZ i uzyty we wszystkich czterech nazwach - patrz
  // serverNameToken. Na Linuksie jest to zawsze sama nazwa serwera.
  const std::string token = serverNameToken(serverName);
  retVal.shmemSegment     = withServerSuffix(kShmemSegment, token);
  retVal.mapMutex         = withServerSuffix(kMapMutex, token);
  retVal.queryQueue       = withServerSuffix(kQueryQueue, token);
  // Prefiks kolejki odpowiedzi domyka się kropką, bo doklejany jest do niego identyfikator
  // klienta: bez separatora "brcdbr.srv" + "12" i "brcdbr.srv1" + "2" dałyby tę samą nazwę.
  retVal.responseQueuePrefix =
      token.empty() ? std::string(kResponseQueuePrefix) : withServerSuffix(kResponseQueuePrefix, token) + ".";
  return retVal;
}

// === Rozmiary buforów i kolejek ===

// Maksymalna liczba wiadomości jednocześnie w RetractorQueryQueue.
constexpr int kQueryQueueMaxMessages = 1000;

// Maksymalny rozmiar pojedynczej komendy w RetractorQueryQueue (bajty).
constexpr int kQueryQueueMaxMessageSize = 1000;

// Maksymalny rozmiar odpowiedzi w kolejce per-proces brcdbr{pid} (bajty).
// Odpowiedzi mogą być dłuższe niż komendy (pełne dane strumieniowe).
constexpr int kResponseQueueMaxMessageSize = 1024;

// Rozmiar segmentu shared memory (bajty). 64 KiB wystarcza na
// wszystkie równoległe odpowiedzi przy typowej liczbie klientów.
constexpr std::size_t kShmemSegmentSize = 65536;

// === Interwały czasowe ===

// Interwał odpytywania kolejek IPC/SPSC - kompromis między latencją a obciążeniem CPU.
// Używany przez: producenta w ipcClient, pętlę komend w executorsm, pętlę select w qry.
constexpr std::chrono::milliseconds kQueuePollInterval{1};

// Czas oczekiwania klienta na odpowiedź serwera w shared memory między próbami.
// Musi być wystarczająco długi przy pracy serwera pod valgrindem (10 prób × 10 ms).
constexpr std::chrono::milliseconds kClientResponsePollInterval{10};

}  // namespace ipc
