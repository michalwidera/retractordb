#include "ipcResponses.hpp"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <sstream>
#include <thread>

#include <spdlog/spdlog.h>
#include <boost/interprocess/permissions.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "bus.hpp"

namespace IPC = boost::interprocess;

namespace ipc::responses {

namespace {

constexpr std::uint32_t raw(SlotState state) { return static_cast<std::uint32_t>(state); }

std::atomic_ref<std::uint32_t> stateOf(Slot &slot) { return std::atomic_ref<std::uint32_t>(slot.state); }

bool transition(Slot &slot, SlotState from, SlotState to) {
  std::uint32_t expected = raw(from);
  return stateOf(slot).compare_exchange_strong(expected, raw(to), std::memory_order_acq_rel, std::memory_order_acquire);
}

// Pola naglowka slotu czyta klient ROWNOLEGLE do zapisu serwera (przeglad przed przejeciem
// slotu), wiec dostep do nich idzie przez atomic_ref - bez tego bylby to wyscig danych w sensie
// modelu pamieci. Porzadek zapewnia stan slotu (release przy READY, acquire przy przejeciu).
template <class T>
T loadField(T &field) {
  return std::atomic_ref<T>(field).load(std::memory_order_relaxed);
}

template <class T>
void storeField(T &field, T value) {
  std::atomic_ref<T>(field).store(value, std::memory_order_relaxed);
}

std::string tooLargeResponse(std::size_t length) {
  boost::property_tree::ptree pt;
  pt.put("error.response", std::format("response too large ({} B > {} B)", length, kResponseSlotDataSize));
  std::stringstream stream;
  write_info(stream, pt);
  return stream.str();
}

// Hak diagnostyczny testu it_ipc_client_killed_reading, ta sama droga co RDB_FAULT_SHOW_DELAY.
// Zatrzymuje klienta w stanie READING, czyli dokladnie tam, gdzie jego smierc zostawia slot
// zajety. Znacznik na stderr mowi testowi, ze klient juz tam stoi - bez zgadywania zegarem.
void faultReadDelay(std::size_t slotIndex) {
  const char *delayMs = std::getenv("RDB_FAULT_CLIENT_READ_DELAY");
  if (delayMs == nullptr) return;
  std::cerr << "RDB_FAULT_CLIENT_READ_DELAY: holding response slot " << slotIndex << " in READING" << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(std::atoi(delayMs)));
}

}  // namespace

void initialize(Segment &segment) {
  segment.header.layoutVersion = kLayoutVersion;
  segment.header.slotCount     = kResponseSlotCount;
  segment.header.slotSize      = sizeof(Slot);
  // Magia na koncu i z release: klient, ktory ja widzi, widzi tez reszte naglowka.
  std::atomic_ref<std::uint64_t>(segment.header.magic).store(kMagic, std::memory_order_release);
}

bool compatible(const Segment &segment) {
  auto &header = const_cast<Header &>(segment.header);  // atomic_ref wymaga typu niestalego
  return std::atomic_ref<std::uint64_t>(header.magic).load(std::memory_order_acquire) == kMagic &&
         header.layoutVersion == kLayoutVersion && header.slotCount == kResponseSlotCount && header.slotSize == sizeof(Slot);
}

PublishStatus publish(Segment &segment, const Owner &owner, std::string_view text) {
  std::string replacement;
  if (text.size() > kResponseSlotDataSize) {
    SPDLOG_ERROR("IPC response for pid {} is {} B, a response slot holds {} B; replaced with an error", owner.pid, text.size(),
                 kResponseSlotDataSize);
    replacement = tooLargeResponse(text.size());
    text        = replacement;
  }

  Slot *target         = nullptr;
  PublishStatus status = PublishStatus::Published;
  for (Slot &slot : segment.slots) {
    if (transition(slot, SlotState::Free, SlotState::Writing)) {
      target = &slot;
      break;
    }
  }

  // Brak wolnego slotu: odzyskujemy slot, ktorego wlasciciel nie zyje. READY to odpowiedz,
  // po ktora nikt juz nie przyjdzie, READING to odbior przerwany smiercia klienta. Zywego
  // wlasciciela nie ruszamy nigdy - jego odbior moze wlasnie trwac.
  for (std::size_t i = 0; target == nullptr && i < kResponseSlotCount; ++i) {
    Slot &slot                   = segment.slots[i];
    const std::uint32_t observed = stateOf(slot).load(std::memory_order_acquire);
    if (observed != raw(SlotState::Ready) && observed != raw(SlotState::Reading)) continue;
    const std::int32_t pid = loadField(slot.pid);
    if (bus::isProcessAlive(pid, loadField(slot.startTime))) continue;
    std::uint32_t expected = observed;
    if (!stateOf(slot).compare_exchange_strong(expected, raw(SlotState::Writing), std::memory_order_acq_rel,
                                               std::memory_order_acquire))
      continue;
    SPDLOG_WARN("IPC response slot {} reclaimed from dead client pid {} ({})", i, pid,
                observed == raw(SlotState::Ready) ? "unread response" : "interrupted read");
    target = &slot;
    status = PublishStatus::Reclaimed;
  }

  if (target == nullptr) {
    SPDLOG_ERROR("IPC response for pid {} dropped: all {} response slots are held by live clients", owner.pid,
                 kResponseSlotCount);
    return PublishStatus::Full;
  }

  storeField(target->pid, owner.pid);
  storeField(target->startTime, owner.startTime);
  storeField(target->seq, owner.seq);
  storeField(target->length, static_cast<std::uint32_t>(text.size()));
  std::memcpy(target->data, text.data(), text.size());
  stateOf(*target).store(raw(SlotState::Ready), std::memory_order_release);
  return status;
}

std::optional<std::string> tryTake(Segment &segment, const Owner &owner) {
  for (std::size_t i = 0; i < kResponseSlotCount; ++i) {
    Slot &slot = segment.slots[i];
    if (stateOf(slot).load(std::memory_order_acquire) != raw(SlotState::Ready)) continue;
    if (loadField(slot.pid) != owner.pid) continue;
    if (!transition(slot, SlotState::Ready, SlotState::Reading)) continue;

    // Pid sprawdzony PRZED przejeciem mogl nalezec do poprzedniego wcielenia slotu: miedzy
    // odczytem a CAS slot mogl przejsc FREE -> WRITING -> READY dla kogos innego. Cudzy slot
    // oddajemy - przez CAS, bo jego martwego wlasciciela serwer mogl juz odzyskac.
    if (loadField(slot.pid) != owner.pid) {
      transition(slot, SlotState::Reading, SlotState::Ready);
      continue;
    }
    // Nasz pid, ale nie nasza odpowiedz: poprzedni proces o tym PID-ie albo nasze wlasne
    // zadanie, na ktore przestalismy czekac. Nikt inny jej nie odbierze, wiec slot zwalniamy.
    if (loadField(slot.startTime) != owner.startTime || loadField(slot.seq) != owner.seq) {
      transition(slot, SlotState::Reading, SlotState::Free);
      continue;
    }

    faultReadDelay(i);
    const std::size_t length = std::min<std::size_t>(loadField(slot.length), kResponseSlotDataSize);
    std::string retVal(static_cast<const char *>(slot.data), length);
    // Nieudane zwolnienie znaczy, ze serwer uznal nas za martwych (startTime nieustalony) i nadpisal
    // slot w trakcie kopiowania - skopiowana tresc jest wtedy niewiarygodna.
    if (!transition(slot, SlotState::Reading, SlotState::Free)) continue;
    return retVal;
  }
  return std::nullopt;
}

std::optional<std::string> take(Segment &segment, const Owner &owner, std::chrono::steady_clock::time_point deadline,
                                std::chrono::milliseconds pollInterval) {
  for (;;) {
    if (auto retVal = tryTake(segment, owner)) return retVal;
    if (std::chrono::steady_clock::now() >= deadline) return std::nullopt;
    std::this_thread::sleep_for(pollInterval);
  }
}

Mapping::Mapping(IPC::create_only_t, const std::string &name)
    : shm_(IPC::create_only, name.c_str(), IPC::read_write, IPC::permissions(kObjectPermissions)) {
  // truncate wypelnia segment zerami, a zero to SlotState::Free w kazdym slocie.
  shm_.truncate(static_cast<IPC::offset_t>(sizeof(Segment)));
  region_ = IPC::mapped_region(shm_, IPC::read_write);
  initialize(segment());
}

Mapping::Mapping(IPC::open_only_t, const std::string &name)
    : shm_(IPC::open_only, name.c_str(), IPC::read_write),
      region_(shm_, IPC::read_write) {}

bool Mapping::valid() const { return region_.get_size() >= sizeof(Segment) && compatible(segment()); }

}  // namespace ipc::responses
