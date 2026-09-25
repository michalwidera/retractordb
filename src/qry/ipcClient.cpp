#include "ipcClient.hpp"

#include <unistd.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include <spdlog/spdlog.h>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/system/system_error.hpp>

#include "constants.hpp"
#include "ipcResponses.hpp"
#include "osPlatform.hpp"

using boost::property_tree::ptree;
namespace IPC = boost::interprocess;

namespace {

/// Numer zadania, liczony na PROCES, nie na obiekt IpcClient: dwa obiekty w jednym procesie
/// maja ten sam pid, wiec liczniki per obiekt dawalyby im te same numery i jeden odbieralby
/// odpowiedz drugiego.
std::atomic<std::uint64_t> requestSeq{0};

/// Znacznik startu tego procesu. Nie zmienia sie przez cale jego zycie, a odczyt to wywolanie jadra.
std::uint64_t selfStartTime() {
  static const std::uint64_t retVal = osplat::inspectProcess(static_cast<std::int32_t>(getpid())).startTime;
  return retVal;
}

}  // namespace

IpcClient::IpcClient(int clientResponseMaxFails, int responseQueueOpenMaxFails, std::string_view serverName)
    : clientResponseMaxFails_(std::max(1, clientResponseMaxFails)),
      responseQueueOpenMaxFails_(std::max(1, responseQueueOpenMaxFails)),
      names_(ipc::names(serverName)) {}

bool IpcClient::popQueue(ptree &pt) { return spsc_queue_.pop(pt); }

void IpcClient::producer() {
  const std::string queueName = names_.responseQueue(getpid());

  // Kolejkę odpowiedzi tworzy SERWER w reakcji na rejestrację klienta, więc
  // `open_only` wołane natychmiast po starcie wątku bywa o krok za wcześnie.
  // Poprzednia wersja poddawała się po pierwszej nieudanej próbie i ustawiała
  // `done`, przez co pętla `select()` nie wykonywała ani jednego obrotu,
  // a klient kończył się kodem 0 bez jednego przeczytanego elementu (issue_215).
  std::unique_ptr<IPC::message_queue> mq;
  int attempts = 0;
  for (; attempts < responseQueueOpenMaxFails_ && !done; ++attempts) {
    try {
      mq = std::make_unique<IPC::message_queue>(IPC::open_only, queueName.c_str());
      break;
    } catch (const IPC::interprocess_exception &) {
      std::this_thread::sleep_for(ipc::kClientResponsePollInterval);
    }
  }
  if (!mq) {
    // Liczba FAKTYCZNYCH prob i powod wyjscia, a nie sam limit. Petla konczy sie takze
    // na `done` ustawionym przez watek glowny, wiec zdanie "po 100 probach" opisywalo
    // wtedy czekanie, ktorego nie bylo, i kierowalo diagnoze na wyscig z serwerem
    // zamiast na przerwanie od strony klienta.
    const bool abortedByClient = done;
    SPDLOG_ERROR("ipcClient: response queue '{}' did not appear after {} of {} attempts ({})", queueName, attempts,
                 responseQueueOpenMaxFails_, abortedByClient ? "aborted by client" : "budget exhausted");
    // Werdykt "serwer nie utworzyl kolejki" ma prawo padac WYLACZNIE po wyczerpaniu budzetu.
    // Producent zerwany przez watek glowny nie czekal, wiec o serwerze nie wie nic -- a mimo to
    // obciazal go na rowni z producentem, ktory przeczekal cale 100 prob. Tak wygladala awaria
    // it_fncall_runtime_case na CI (2026-09-04): klient konczyl petle na bajcie z terminala,
    // a meldowal brak kolejki odpowiedzi i wskazywal winnego po drugiej stronie IPC.
    if (!abortedByClient) responseQueueMissing = true;
    done = true;
    return;
  }

  try {
    std::array<char, ipc::kResponseQueueMaxMessageSize + ipc::kNullTerminatorBytes> message;
    unsigned int priority{0};
    IPC::message_queue::size_type recvd_size = ipc::kResponseQueueMaxMessageSize;
    while (!done) {
      bool messageReceived = false;
      // Interwal odpytywania nalezy do kolejki PUSTEJ. Sen po UDANYM odbiorze narzucal
      // tempo jednego wiersza na milisekunde niezaleznie od zaleglosci, wiec klient, ktory
      // zostal w tyle, nie mial jak nadrobic: nadganianie szlo dokladnie tak wolno, jak
      // szedl biezacy strumien. Oproznienie pelnej kolejki (1024 wiersze) kosztowalo przez
      // to ponad sekunde samego spania, i to na kazdym czekaniu, ktore jest juz obsluzone.
      while (!messageReceived && !done) {
        messageReceived = mq->try_receive(message.data(), ipc::kResponseQueueMaxMessageSize, recvd_size, priority);
        if (!messageReceived) std::this_thread::sleep_for(ipc::kQueuePollInterval);
      }
      if (done) continue;
      message[recvd_size] = 0;
      std::stringstream strstream;
      strstream << message.data();
      memset(message.data(), 0, ipc::kResponseQueueMaxMessageSize);
      ptree pt;
      read_info(strstream, pt);
      while (!done && !spsc_queue_.push(pt))
        std::this_thread::sleep_for(ipc::kQueuePollInterval);
    }
  } catch (const std::exception &e) {
    SPDLOG_ERROR("IPC: {} (producer queue:{})", e.what(), names_.responseQueue(getpid()));
    done = true;
  }
}

ptree IpcClient::netClient(const std::string &netCommand, const std::string &netArgument) {
  ptree pt_response;
  ptree pt_request;
  try {
    const ipc::responses::Mapping responses(IPC::open_only, names_.shmemSegment);
    if (!responses.valid()) {
      SPDLOG_ERROR("ipcClient: response segment '{}' has an incompatible layout", names_.shmemSegment);
      done = true;
      pt_response.put("error.response", "server not found");
      return pt_response;
    }
    const ipc::responses::Owner self{
        .pid = static_cast<std::int32_t>(getpid()), .startTime = selfStartTime(), .seq = ++requestSeq};
    pt_request.put("db.message", netCommand);
    pt_request.put("db.id", self.pid);
    pt_request.put("db.seq", self.seq);
    if (!netArgument.empty()) pt_request.put("db.argument", netArgument);

    // Budżet liczony ZEGAREM, a nie liczbą obrotów pętli: na obciążonej maszynie obrót trwa
    // dłużej niż sam interwał, a wtedy liczenie prób skracało faktyczne czekanie dokładnie
    // w sytuacji, w której potrzebne było najdłuższe (issue_217). Jeden termin obejmuje oba
    // czekania - na miejsce w kolejce komend i na odpowiedź - więc żadne nie trwa bez końca.
    const auto deadline = std::chrono::steady_clock::now() + clientResponseMaxFails_ * ipc::kClientResponsePollInterval;

    IPC::message_queue mq(IPC::open_only, names_.queryQueue.c_str());
    std::stringstream request_stream;
    write_info(request_stream, pt_request);
    const std::string request = request_stream.str();
    if (!mq.timed_send(request.c_str(), request.length(), 0, deadline)) {
      SPDLOG_ERROR("ipcClient: command queue '{}' stayed full until the deadline", names_.queryQueue);
      done = true;
      pt_response.put("error.response", "server not found");
      return pt_response;
    }

    const auto response = ipc::responses::take(responses.segment(), self, deadline, ipc::kClientResponsePollInterval);
    if (!response) {
      SPDLOG_ERROR("server not found");
      done = true;
      pt_response.put("error.response", "server not found");
      return pt_response;
    }
    std::stringstream strstream(*response);
    read_info(strstream, pt_response);
  } catch (IPC::interprocess_exception &e) {
    done = true;
    throw;
  } catch (boost::system::system_error &e) {
    done = true;
    throw;
  } catch (std::exception &e) {
    pt_response.put("error.response", e.what());
    done = true;
    throw;
  }
  return pt_response;
}
