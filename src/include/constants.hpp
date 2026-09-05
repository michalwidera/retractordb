#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>

namespace constants {
constexpr std::string_view Reserved_id_oob = "OUT_OF_BUSSINESS";

/// Odpowiedz serwera na komende wymagajaca modelu danych, gdy instancja nie ma wczytanego
/// planu (tryb bezczynny). Wspolna dla obu stron IPC: serwer ja wpisuje, klient rozpoznaje.
/// Bez wspolnej stalej klient nie odroznial instancji bezczynnej od serwera, ktory nie
/// odpowiedzial — i meldowal timeout tam, gdzie odpowiedz przyszla od razu.
constexpr std::string_view kNoActivePlanReply = "no active plan";
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
  retVal.shmemSegment = withServerSuffix(kShmemSegment, serverName);
  retVal.mapMutex     = withServerSuffix(kMapMutex, serverName);
  retVal.queryQueue   = withServerSuffix(kQueryQueue, serverName);
  // Prefiks kolejki odpowiedzi domyka się kropką, bo doklejany jest do niego identyfikator
  // klienta: bez separatora "brcdbr.srv" + "12" i "brcdbr.srv1" + "2" dałyby tę samą nazwę.
  retVal.responseQueuePrefix =
      serverName.empty() ? std::string(kResponseQueuePrefix) : withServerSuffix(kResponseQueuePrefix, serverName) + ".";
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

// Interwał odpytywania kolejek IPC/SPSC — kompromis między latencją a obciążeniem CPU.
// Używany przez: producenta w ipcClient, pętlę komend w executorsm, pętlę select w qry.
constexpr std::chrono::milliseconds kQueuePollInterval{1};

// Czas oczekiwania klienta na odpowiedź serwera w shared memory między próbami.
// Musi być wystarczająco długi przy pracy serwera pod valgrindem (10 prób × 10 ms).
constexpr std::chrono::milliseconds kClientResponsePollInterval{10};

}  // namespace ipc
