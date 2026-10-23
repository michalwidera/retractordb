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

// NOLINTNEXTLINE(performance-enum-size): zgodny z 32-bitowym slowem atomowym stanu slotu IPC
enum class SlotState : std::uint32_t { Free = 0, Writing = 1, Ready = 2, Reading = 3 };

/// @requirements-exempt Header: Naglowek danych segmentu IPC; inicjalizacje i kontrole wykonuja funkcje protokolu.
struct Header {
  std::uint64_t magic;
  std::uint32_t layoutVersion;
  std::uint32_t slotCount;
  std::uint32_t slotSize;
  std::uint32_t reserved0;
  std::uint64_t reserved1;
};

/// @requirements-exempt Slot: Rekord odpowiedzi IPC; przejscia stanu realizuja funkcje protokolu.
struct Slot {
  std::uint32_t state;  ///< SlotState, dostep wylacznie przez std::atomic_ref
  std::int32_t pid;
  std::uint64_t startTime;  ///< znacznik startu klienta z db.start; bez niego odczytany przez serwer
  std::uint64_t seq;        ///< db.seq zadania, na ktore to jest odpowiedz
  std::uint32_t length;
  std::uint32_t reserved;
  char data[kResponseSlotDataSize];  // NOLINT(modernize-avoid-c-arrays): uklad pamieci dzielonej
};

/// @requirements-exempt Segment: Uklad pamieci grupujacy naglowek i sloty odpowiedzi IPC.
struct Segment {
  Header header;
  Slot slots[kResponseSlotCount];  // NOLINT(modernize-avoid-c-arrays): uklad pamieci dzielonej
};

static_assert(std::is_trivially_copyable_v<Segment> && std::is_standard_layout_v<Segment>);
static_assert(sizeof(Header) == kResponseSegmentHeaderBytes);
static_assert(sizeof(Slot) == kResponseSlotHeaderBytes + kResponseSlotDataSize);
static_assert(sizeof(Segment) == kShmemSegmentSize);

/// Adresat odpowiedzi: proces klienta i numer jego zadania.
/// @requirements-exempt Owner: Agregat danych adresata odpowiedzi IPC, bez wlasnej logiki.
struct Owner {
  std::int32_t pid{0};
  std::uint64_t startTime{0};
  std::uint64_t seq{0};
};

/// Numer zadania tego procesu, na ktorego odpowiedz jakis watek wlasnie czeka. Rejestr jest wspolny
/// dla calego procesu, jak licznik requestSeq w ipcClient.cpp: bez niego tryTake jednego watku
/// zwalnial odpowiedz zadania, na ktora drugi watek tego samego procesu jeszcze czekal.
/// Obiekt klasy PendingRequest powinien:
/// - [ipcresponses.pending-request.1~1] od konstrukcji do zniszczenia oznaczać numer `seq` jako żądanie w toku tego procesu, tak aby tryTake innego żądania nie zwolnił jego odpowiedzi.
/// - [ipcresponses.pending-request.2~1] przy zniszczeniu wycofywać swój numer, po czym zaległa odpowiedź na niego jest zwalniana przez tryTake jak każda porzucona.
class PendingRequest {
 public:
  explicit PendingRequest(std::uint64_t seq);
  ~PendingRequest();
  PendingRequest(const PendingRequest &)            = delete;
  PendingRequest &operator=(const PendingRequest &) = delete;

 private:
  std::uint64_t seq_;
};

enum class PublishStatus : std::uint8_t {
  Published,  ///< w wolnym slocie
  Reclaimed,  ///< w slocie odzyskanym po martwym kliencie
  Full        ///< wszystkie sloty zajete przez zywych - odpowiedz przepada
};

/// Funkcja initialize powinna:
/// - [ipcresponses.initialize.1~1] zapisywać w nagłówku wyzerowanego segmentu wersję układu kLayoutVersion, liczbę slotów kResponseSlotCount i rozmiar slotu, a magię kMagic na końcu z semantyką release, tak aby klient widzący magię widział cały nagłówek.
///   Nietestowalne: porzadek release/acquire nie wynika z testu wartosci; kontrola strukturalna initialize/compatible w ipcResponses.cpp (magia zapisywana na koncu).
/// - [ipcresponses.initialize.2~1] pozostawiać sloty w stanie Free.
void initialize(Segment &segment);

/// Funkcja compatible powinna:
/// - [ipcresponses.compatible.1~1] zwracać true wyłącznie wtedy, gdy nagłówek segmentu ma magię kMagic (odczytaną z semantyką acquire), wersję układu kLayoutVersion, liczbę slotów kResponseSlotCount i rozmiar slotu tej binarki.
[[nodiscard]] bool compatible(const Segment &segment);

/// Funkcja publish powinna:
/// - [ipcresponses.publish.1~1] zastępować odpowiedź dłuższą niż kResponseSlotDataSize komunikatem info z error.response "response too large (N B > K B)" i logować to jako błąd.
/// - [ipcresponses.publish.2~1] zapisywać odpowiedź w pierwszym wolnym slocie razem z pid, startTime i seq adresata, a stan Ready ustawiać dopiero po zapisaniu treści, i zwracać Published.
///   Nietestowalne: kolejnosc zapisu payloadu przed release Ready - kontrola strukturalna publish w ipcResponses.cpp.
/// - [ipcresponses.publish.3~1] przy braku wolnego slotu odzyskiwać slot w stanie Ready albo Reading, którego właściciel nie żyje, i zwracać Reclaimed, nigdy nie odbierając slotu żywemu klientowi.
///   Zakres testow: `IpcResponses.ready_slot_of_dead_client_is_reclaimed_when_full`, `IpcResponses.reading_slot_of_dead_client_is_reclaimed_when_full`, `IpcResponses.live_owners_are_never_reclaimed`, `IpcResponses.live_readers_are_not_reclaimed_or_modified`; integracja `it_ipc_client_killed_reading-run`.
/// - [ipcresponses.publish.4~1] gdy wszystkie sloty trzymają żywi klienci, porzucać odpowiedź z logiem błędu i zwracać Full.
///
/// @note Wolac wylacznie z jednego watku.
PublishStatus publish(Segment &segment, const Owner &owner, std::string_view text);

/// Funkcja tryTake powinna:
/// - [ipcresponses.try-take.1~1] w jednym przeglądzie slotów zwracać treść odpowiedzi w stanie Ready z pid, startTime i seq równymi `owner` i zwalniać jej slot, a bez takiej odpowiedzi zwracać nullopt.
/// - [ipcresponses.try-take.2~2] zwalniać po drodze sloty z pid-em `owner`, ale innym startTime (zaległe odpowiedzi poprzedniego procesu) albo innym seq, który nie jest żądaniem w toku tego procesu (porzucone żądania); slot żądania w toku (PendingRequest) oddawać w stanie Ready jego wątkowi.
/// - [ipcresponses.try-take.3~1] oddawać w stanie Ready slot, który po przejęciu okazał się należeć do innego pid-u.
///   Nietestowalne: deterministyczna podmiana pid miedzy wstepnym odczytem a CAS nie ma punktu synchronizacji. Kontrola strukturalna tryTake w ipcResponses.cpp: drugi odczyt pid po CAS i oddanie Reading -> Ready przez CAS. Test `IpcResponses.foreign_pid_slot_is_left_alone` sprawdza tylko sciezke bez podmiany; nie zalicza sie jako test tego wyscigu.
/// - [ipcresponses.try-take.4~1] odrzucać skopiowaną treść, gdy zwolnienie slotu po kopiowaniu się nie powiedzie, bo serwer mógł go w tym czasie nadpisać.
///   Zakres testow: `IpcResponses.failed_release_discards_the_copy_and_keeps_scanning` (przechwycenie istniejacego getenv punktu diagnostycznego po przejeciu slotu, bez haka w produkcji).
[[nodiscard]] std::optional<std::string> tryTake(Segment &segment, const Owner &owner);

/// Funkcja take powinna:
/// - [ipcresponses.take.1~1] powtarzać tryTake co pollInterval i zwracać pierwszą odebraną odpowiedź, a po osiągnięciu deadline bez odpowiedzi zwracać nullopt.
[[nodiscard]] std::optional<std::string> take(Segment &segment, const Owner &owner,
                                              std::chrono::steady_clock::time_point deadline,
                                              std::chrono::milliseconds pollInterval);

/// Odwzorowanie segmentu odpowiedzi. Konstrukcja z create_only tworzy go i inicjuje (serwer),
/// z open_only otwiera istniejacy (klient). Bledy systemowe jako interprocess_exception.
/// Obiekt klasy Mapping powinien:
/// - [ipcresponses.mapping.1~1] w konstruktorze z create_only tworzyć nowy obiekt pamięci dzielonej przez ipc::security::createMemory, odmawiając istniejącej nazwy, ustawiać jego rozmiar na sizeof(Segment), odwzorowywać go do odczytu i zapisu i inicjować nagłówek segmentu przez initialize, pozostawiając wszystkie sloty w stanie Free.
/// - [ipcresponses.mapping.2~1] w konstruktorze z open_only otwierać istniejący obiekt przez ipc::security::openMemory z kontrolą właściciela, trybu i dowiązań oraz odwzorowywać go do odczytu i zapisu bez zmiany jego zawartości.
/// - [ipcresponses.mapping.3~1] zgłaszać błędy tworzenia, otwarcia i odwzorowania segmentu wyjątkiem interprocess_exception albo ipc::security::Rejected, bez zwracania częściowo odwzorowanego obiektu.
///   Zakres testow: `IpcResponses.mapping_refuses_missing_and_existing_names_without_replacing_the_owner`, `IpcResponses.mapping_checks_size_and_every_header_field_without_reinitializing_slots` (w tym porazka mapowania pustego obiektu).
/// - [ipcresponses.mapping.4~1] przez valid zwracać prawdę wyłącznie wtedy, gdy odwzorowanie ma co najmniej sizeof(Segment) bajtów, a nagłówek ma magię kMagic i wersję układu, liczbę i rozmiar slotów tej binarki.
/// - [ipcresponses.mapping.5~1] przez segment udostępniać odwzorowany Segment bez kopiowania, pozostawiając protokół slotów funkcjom publish, tryTake i take.
/// - [ipcresponses.mapping.6~1] utrzymywać odwzorowanie i uchwyt obiektu przez czas życia Mapping i zwalniać je przy zniszczeniu bez usuwania nazwy obiektu pamięci dzielonej.
///   Nietestowalne: zamkniecie uchwytu i unmap sa delegowane destruktorom pol Boosta; kontrola strukturalna skladu Mapping, bez recznego zarzadzania uchwytami.
class Mapping {
 public:
  Mapping(boost::interprocess::create_only_t /*tag*/, const std::string &name);
  Mapping(boost::interprocess::open_only_t /*tag*/, const std::string &name);

  /// Falsz, gdy odwzorowanie jest krotsze od ukladu albo naglowek nie pasuje.
  [[nodiscard]] bool valid() const;
  [[nodiscard]] Segment &segment() const { return *static_cast<Segment *>(region_.get_address()); }

 private:
  boost::interprocess::shared_memory_object shm_;
  boost::interprocess::mapped_region region_;
};

}  // namespace ipc::responses
