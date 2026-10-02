#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

#include "constants.hpp"

/// @brief Sloty odpowiedzi na komendy IPC: serwer odklada odpowiedz, klient ja odbiera.
///
/// Segment zawiera WYLACZNIE typy POD i tablice o stalym rozmiarze, bez zadnego zamka. Wczesniej
/// byla tu boost::container::map z alokatorem w pamieci dzielonej pod named_mutex: proces zabity
/// z muteksem w reku zawieszal obsluge komend na zawsze (named_mutex nie jest robust), a zabity
/// w trakcie wstawiania do mapy zostawial sterte, ktorej nie da sie naprawic. Tu smierc procesu
/// zostawia co najwyzej jeden slot w stanie posrednim, a taki slot serwer odzyskuje.
///
/// Stan slotu to jedno slowo atomowe, przejscia wylacznie przez compare_exchange:
///
///   FREE -> WRITING -> READY          wylacznie watek komunikacyjny serwera (jeden pisarz,
///                                     wiec wybor wolnego slotu nie potrzebuje zamka)
///   READY -> READING -> FREE          klient, wylacznie w slocie ze swoim pid
///   READY|READING -> WRITING          serwer, gdy brak wolnego slotu, a wlasciciel slotu nie zyje
///
/// Pola pid, startTime, seq, length i data pisze wylacznie serwer, i tylko w stanie WRITING.
/// Wlasciciela wyznacza para (pid, startTime), a nie sam pid, bo PID-y sa reuzywane: nowy proces
/// z PID-em zmarlego klienta liczy seq od nowa i bez startTime przyjalby jego zalegla odpowiedz.
namespace ipc::responses {

inline constexpr std::uint64_t kMagic         = 0x5244'4252'4553'5031ULL;  // "RDBRESP1"
inline constexpr std::uint32_t kLayoutVersion = 1;

enum class SlotState : std::uint32_t { Free = 0, Writing = 1, Ready = 2, Reading = 3 };

struct Header {
  std::uint64_t magic;
  std::uint32_t layoutVersion;
  std::uint32_t slotCount;
  std::uint32_t slotSize;
  std::uint32_t reserved0;
  std::uint64_t reserved1;
};

struct Slot {
  std::uint32_t state;  ///< SlotState, dostep wylacznie przez std::atomic_ref
  std::int32_t pid;
  std::uint64_t startTime;  ///< znacznik startu klienta odczytany przez serwer w chwili zapisu
  std::uint64_t seq;        ///< db.seq zadania, na ktore to jest odpowiedz
  std::uint32_t length;
  std::uint32_t reserved;
  char data[kResponseSlotDataSize];  // NOLINT(modernize-avoid-c-arrays): uklad pamieci dzielonej
};

struct Segment {
  Header header;
  Slot slots[kResponseSlotCount];  // NOLINT(modernize-avoid-c-arrays): uklad pamieci dzielonej
};

static_assert(std::is_trivially_copyable_v<Segment> && std::is_standard_layout_v<Segment>);
static_assert(sizeof(Header) == kResponseSegmentHeaderBytes);
static_assert(sizeof(Slot) == kResponseSlotHeaderBytes + kResponseSlotDataSize);
static_assert(sizeof(Segment) == kShmemSegmentSize);

/// Adresat odpowiedzi: proces klienta i numer jego zadania.
struct Owner {
  std::int32_t pid{0};
  std::uint64_t startTime{0};
  std::uint64_t seq{0};
};

enum class PublishStatus {
  Published,  ///< w wolnym slocie
  Reclaimed,  ///< w slocie odzyskanym po martwym kliencie
  Full        ///< wszystkie sloty zajete przez zywych - odpowiedz przepada
};

/// Wypelnia naglowek swiezo utworzonego (wyzerowanego) segmentu.
void initialize(Segment &segment);

/// Czy segment ma uklad tej binarki.
[[nodiscard]] bool compatible(const Segment &segment);

/// Odpowiedz na komende serwera - odpowiedz dluzsza niz slot jest zastepowana
/// error.response "response too large (N B > K B)". Wolac wylacznie z jednego watku.
PublishStatus publish(Segment &segment, const Owner &owner, std::string_view text);

/// Jeden przeglad slotow: odpowiedz dla `owner` albo nullopt. Sloty z pid-em `owner`, ale
/// innym seq albo startTime (zalegle odpowiedzi) zwalnia po drodze.
[[nodiscard]] std::optional<std::string> tryTake(Segment &segment, const Owner &owner);

/// tryTake powtarzane co `pollInterval` az do `deadline`.
[[nodiscard]] std::optional<std::string> take(Segment &segment, const Owner &owner,
                                              std::chrono::steady_clock::time_point deadline,
                                              std::chrono::milliseconds pollInterval);

/// Odwzorowanie segmentu odpowiedzi. Konstrukcja z create_only tworzy go i inicjuje (serwer),
/// z open_only otwiera istniejacy (klient). Bledy systemowe jako interprocess_exception.
class Mapping {
 public:
  Mapping(boost::interprocess::create_only_t, const std::string &name);
  Mapping(boost::interprocess::open_only_t, const std::string &name);

  /// Falsz, gdy odwzorowanie jest krotsze od ukladu albo naglowek nie pasuje.
  [[nodiscard]] bool valid() const;
  [[nodiscard]] Segment &segment() const { return *static_cast<Segment *>(region_.get_address()); }

 private:
  boost::interprocess::shared_memory_object shm_;
  boost::interprocess::mapped_region region_;
};

}  // namespace ipc::responses
