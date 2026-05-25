#pragma once

#include <chrono>
#include <cstddef>
#include <string_view>

namespace constants {
constexpr std::string_view Reserved_id_oob = "OUT_OF_BUSSINESS";
}  // namespace constants

namespace ipc {

// === Shared memory / mutex / queue names ===
// Muszą być spójne między serwerem (executorsm) a klientem (ipc_transport, qryLauncher).

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
// Używany przez: producenta w ipc_transport, pętlę komend w executorsm, pętlę select w qry.
constexpr std::chrono::milliseconds kQueuePollInterval{1};

// Czas oczekiwania klienta na odpowiedź serwera w shared memory między próbami.
// Musi być wystarczająco długi przy pracy serwera pod valgrindem (10 prób × 10 ms).
constexpr std::chrono::milliseconds kClientResponsePollInterval{10};

}  // namespace ipc
