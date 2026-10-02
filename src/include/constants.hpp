#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <optional>
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

// === Shared memory / queue names ===
// Muszą być spójne między serwerem (executorsm) a klientem (ipcClient, qryLauncher).

// Segment pamieci dzielonej ze slotami odpowiedzi na komendy (uklad w ipcResponses.hpp).
// Nazwa niesie wersje ukladu z tego samego powodu co segment magistrali (bus.hpp): segment
// starego ukladu zostaje w /dev/shm po podmianie binarki i nie moze byc czytany nowym.
// Podkreslenie, nie kropka - kropka oddziela nazwe instancji. Nie dluzsza niz 17 znakow:
// z kropka, skrotem nazwy instancji (9) i ukosnikiem Boosta ma sie zmiescic w 31 znakach
// Darwina (kMaxObjectNameLength); "RetractorResponses_v1" juz sie nie miescila.
constexpr std::string_view kShmemSegment = "RetractorReply_v1";

// Główna kolejka komend: klient wysyła, serwer odbiera.
constexpr std::string_view kQueryQueue = "RetractorQueryQueue";

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
  std::string queryQueue;
  std::string responseQueuePrefix;

  /// Nazwa kolejki odpowiedzi konkretnego klienta.
  [[nodiscard]] std::string responseQueue(int clientId) const { return responseQueuePrefix + std::to_string(clientId); }
};

/// Najdluzsza nazwa obiektu IPC, ktora jadro przyjmie.
///
/// Na jadrach BSD-owych nazwy POSIX-owych semaforow i obiektow pamieci dzielonej
/// sa ograniczone do PSEMNAMLEN / PSHMNAMLEN, czyli 31 znakow, a dluzsza konczy
/// sie ENAMETOOLONG ("File name too long") juz przy TWORZENIU obiektu. Na Linuksie
/// limitem jest NAME_MAX (255) i zapas jest tak duzy, ze
/// warunek ponizej nigdy nie zadziala - nazwy zostaja doslownie takie jak dotad.
inline constexpr std::size_t kMaxObjectNameLength = RDB_OS_DARWIN ? 31 : 200;

/// Zapas na czlon doklejany PO zlozeniu nazwy bazowej: identyfikator klienta
/// w nazwie kolejki odpowiedzi plus ukosnik, ktory Boost stawia z przodu.
inline constexpr std::size_t kObjectNameTailBudget = 12;

/// Dlugosc skrotu z shortServerTag: "0" i osiem cyfr szesnastkowych.
inline constexpr std::size_t kShortServerTagLength = 9;

/// Skrot nazwy serwera: osiem cyfr szesnastkowych FNV-1a.
///
/// Liczony JAWNIE, a nie przez std::hash, z tego samego powodu co skrot sciezki
/// magazynu w bus.hpp: wartosc musi byc identyczna po obu stronach IPC, a wynik
/// std::hash jest szczegolem implementacji biblioteki standardowej.
inline std::string shortServerTag(std::string_view serverName) {
  constexpr std::uint64_t basis = 0xcbf2'9ce4'8422'2325ULL;
  constexpr std::uint64_t prime = 0x0000'0100'0000'01B3ULL;
  constexpr unsigned halfBits   = 32;  // skrot 32-bitowy: polowki 64-bitowego XOR-owane
  std::uint64_t hash            = basis;
  for (const unsigned char byte : serverName) {
    hash ^= byte;
    hash *= prime;
  }
  // NOLINTNEXTLINE(modernize-avoid-c-arrays): bufor dla snprintf
  char buffer[kShortServerTagLength + 1];
  // Nazwa uzytkownika zaczyna sie litera, wiec 0 oddziela skroty od nazw doslownych.
  std::snprintf(buffer, sizeof(buffer), "0%08x", static_cast<unsigned>(hash ^ (hash >> halfBits)));
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

/// Komplet nazw dla gotowego czlonu instancji (patrz serverNameToken). Sprzatacz pozostalosci
/// zna wylacznie czlon - odczytany z nazwy pliku blokady tozsamosci - a nie nazwe serwera.
inline ServerNames namesForToken(std::string_view token) {
  ServerNames retVal;
  retVal.shmemSegment = withServerSuffix(kShmemSegment, token);
  retVal.queryQueue   = withServerSuffix(kQueryQueue, token);
  // Prefiks kolejki odpowiedzi domyka się kropką, bo doklejany jest do niego identyfikator
  // klienta: bez separatora "brcdbr.srv" + "12" i "brcdbr.srv1" + "2" dałyby tę samą nazwę.
  retVal.responseQueuePrefix =
      token.empty() ? std::string(kResponseQueuePrefix) : withServerSuffix(kResponseQueuePrefix, token) + ".";
  return retVal;
}

/// Katalog blokad wspolnych dla calej maszyny: tozsamosci IPC i obecnosci na magistrali.
/// Nie TMPDIR: obiekty chronione tymi blokadami widac ze wszystkich katalogow tymczasowych.
inline constexpr std::string_view kMachineLockDir = "/tmp";

/// Przedrostek pliku blokady tozsamosci IPC. Pelna nazwa: przedrostek + nazwa kolejki komend + ".lock".
inline constexpr std::string_view kIdentityLockPrefix = "xretractor_ipc.";

/// Plik blokady tozsamosci IPC. Serwer trzyma ja wylacznie od chwili PRZED utworzeniem swoich
/// obiektow IPC do chwili PO ich skasowaniu, a po jego smierci zwalnia ja jadro - dlatego jest
/// zarazem najpewniejszym dowodem, ze serwer o tym czlonie zyje.
inline std::string identityLockPath(std::string_view queryQueue) {
  return std::string(kMachineLockDir) + "/" + std::string(kIdentityLockPrefix) + std::string(queryQueue) + ".lock";
}

/// Czlon instancji liczony RAZ i uzyty we wszystkich czterech nazwach - patrz serverNameToken.
/// Na Linuksie jest to zawsze sama nazwa serwera.
inline ServerNames names(std::string_view serverName = {}) { return namesForToken(serverNameToken(serverName)); }

/// Rodzina plikow blokady instancji: xretractor_service.lock dla instancji bezimiennej,
/// xretractor_service.<nazwa serwera>.lock dla nazwanej. Przypieta na stale, a nie brana z argv[0]:
/// te sama rodzine przegladaja sprzatacz pozostalosci i straznik xtrdb, a zaden z nich nie zna
/// nazwy, pod jaka uruchomiono silnik.
inline constexpr std::string_view kServiceLockFamily = "xretractor_service";
inline constexpr std::string_view kServiceLockSuffix = ".lock";

/// Nazwa uslugi instancji: rdzen nazwy pliku jej blokady i etykieta w `xretractor --status`.
/// Czlonem jest sama nazwa serwera, nie skrot z serverNameToken.
inline std::string serviceName(std::string_view serverName) { return withServerSuffix(kServiceLockFamily, serverName); }

/// Plik blokady instancji (bez katalogu).
inline std::string serviceLockFile(std::string_view serviceName) {
  return std::string(serviceName) + std::string(kServiceLockSuffix);
}

/// Katalog blokad instancji: paths.lock_dir, a gdy nie ustawiony - katalog tymczasowy procesu
/// (TMPDIR). Inaczej niz kMachineLockDir: ta blokada rozdziela instancje w obrebie jednego katalogu.
inline std::filesystem::path serviceLockDir(std::string_view configuredDir) {
  if (configuredDir.empty()) return std::filesystem::temp_directory_path();
  return std::filesystem::path(configuredDir);
}

/// Czlon serwera odczytany z nazwy pliku blokady instancji: pusty dla instancji bezimiennej,
/// nullopt dla pliku spoza rodziny. Poprawnosci samej nazwy nie ocenia - to nalezy do wolajacego.
inline std::optional<std::string_view> serviceLockInstance(std::string_view file) {
  if (file.size() < kServiceLockFamily.size() + kServiceLockSuffix.size() || !file.starts_with(kServiceLockFamily) ||
      !file.ends_with(kServiceLockSuffix))
    return std::nullopt;
  const std::string_view middle =
      file.substr(kServiceLockFamily.size(), file.size() - kServiceLockFamily.size() - kServiceLockSuffix.size());
  if (middle.empty()) return middle;
  if (middle.size() < 2 || middle.front() != '.') return std::nullopt;
  return middle.substr(1);
}

// === Rozmiary buforów i kolejek ===

// Maksymalna liczba wiadomości jednocześnie w RetractorQueryQueue.
constexpr int kQueryQueueMaxMessages = 1000;

// Maksymalny rozmiar pojedynczej komendy w RetractorQueryQueue (bajty).
constexpr int kQueryQueueMaxMessageSize = 1000;

// Maksymalny rozmiar odpowiedzi w kolejce per-proces brcdbr{pid} (bajty).
// Odpowiedzi mogą być dłuższe niż komendy (pełne dane strumieniowe).
constexpr int kResponseQueueMaxMessageSize = 1024;

// Miejsce na terminator w buforze odbiorczym kazdej z dwoch kolejek. try_receive moze oddac
// DOKLADNIE max_message_size bajtow, a odbiorca pisze '\0' pod indeksem recvd_size, czyli
// zaraz za nimi. Bufor o rozmiarze samego max_message_size konczy sie zapisem poza tablica.
constexpr std::size_t kNullTerminatorBytes = 1;

// Liczba slotow odpowiedzi w segmencie kShmemSegment, czyli liczba odpowiedzi, ktore moga
// czekac na odbior jednoczesnie. Slot zajety przez martwego klienta serwer odzyskuje.
constexpr std::size_t kResponseSlotCount = 16;

// Najwieksza odpowiedz miesczaca sie w slocie (bajty). Dluzsza jest zastepowana bledem.
constexpr std::size_t kResponseSlotDataSize = 32 * 1024;

// Naglowek segmentu i naglowek slotu (bajty); sizeof obu struktur pilnuje static_assert
// w ipcResponses.hpp.
constexpr std::size_t kResponseSegmentHeaderBytes = 32;
constexpr std::size_t kResponseSlotHeaderBytes    = 32;

// Rozmiar segmentu odpowiedzi (bajty): naglowek plus kResponseSlotCount pelnych slotow.
constexpr std::size_t kShmemSegmentSize =
    kResponseSegmentHeaderBytes + kResponseSlotCount * (kResponseSlotHeaderBytes + kResponseSlotDataSize);

// === Uprawnienia obiektów IPC ===

// Tryb nadawany KAŻDEMU obiektowi IPC, który tworzy serwer: segmentowi odpowiedzi,
// kolejce komend, kolejkom odpowiedzi i segmentowi magistrali.
//
// Jawny, bo domyślny `permissions()` Boosta deklaruje 0666, a tryb realny wychodzi dopiero
// spod umaska procesu: przy umask 022 jest to 0644 (obcy użytkownik czyta segment, choć do
// kolejki pisać nie może), a przy umask 002 już 0664 - czyli cała grupa może wysyłać komendy,
// w tym `--reset`, który wymienia cały plan. Ochrona zależna od umaska jednostki systemd nie
// jest ochroną, a od niej zależy sensowność trybu `service.unrestricted`.
//
// 0600 nie zabiera niczego klientowi: wszystkie te obiekty tworzy serwer, a xqry otwiera je
// wyłącznie przez open_only (ipcClient.cpp, qryLauncher.cpp), więc klient i tak musi działać
// na koncie serwera. Bity właściciela przeżywają każdy sensowny umask.
constexpr int kObjectPermissions = 0600;

// === Interwały czasowe ===

// Interwał odpytywania kolejek IPC/SPSC - kompromis między latencją a obciążeniem CPU.
// Używany przez: producenta w ipcClient, pętlę komend w executorsm, pętlę select w qry.
constexpr std::chrono::milliseconds kQueuePollInterval{1};

// Czas oczekiwania klienta na odpowiedź serwera w shared memory między próbami.
// Musi być wystarczająco długi przy pracy serwera pod valgrindem (10 prób × 10 ms).
constexpr std::chrono::milliseconds kClientResponsePollInterval{10};

}  // namespace ipc
