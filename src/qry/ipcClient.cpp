#include "ipcClient.hpp"

#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include <spdlog/spdlog.h>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/system/system_error.hpp>

#include "constants.hpp"
#include "ipcTypes.hpp"

using boost::property_tree::ptree;
namespace IPC = boost::interprocess;

IpcClient::IpcClient(int clientResponseMaxFails, int responseQueueOpenMaxFails)
    : clientResponseMaxFails_(std::max(1, clientResponseMaxFails)),
      responseQueueOpenMaxFails_(std::max(1, responseQueueOpenMaxFails)) {}

bool IpcClient::popQueue(ptree &pt) { return spsc_queue_.pop(pt); }

void IpcClient::producer() {
  const std::string queueName = std::string(ipc::kResponseQueuePrefix) + std::to_string(getpid());

  // Kolejkę odpowiedzi tworzy SERWER w reakcji na rejestrację klienta, więc
  // `open_only` wołane natychmiast po starcie wątku bywa o krok za wcześnie.
  // Poprzednia wersja poddawała się po pierwszej nieudanej próbie i ustawiała
  // `done`, przez co pętla `select()` nie wykonywała ani jednego obrotu,
  // a klient kończył się kodem 0 bez jednego przeczytanego elementu (issue_215).
  std::unique_ptr<IPC::message_queue> mq;
  for (int attempt = 0; attempt < responseQueueOpenMaxFails_ && !done; ++attempt) {
    try {
      mq = std::make_unique<IPC::message_queue>(IPC::open_only, queueName.c_str());
      break;
    } catch (const IPC::interprocess_exception &) {
      std::this_thread::sleep_for(ipc::kClientResponsePollInterval);
    }
  }
  if (!mq) {
    SPDLOG_ERROR("ipcClient: response queue '{}' did not appear after {} attempts", queueName, responseQueueOpenMaxFails_);
    responseQueueMissing = true;
    done                 = true;
    return;
  }

  try {
    std::array<char, ipc::kResponseQueueMaxMessageSize> message;
    unsigned int priority{0};
    IPC::message_queue::size_type recvd_size = ipc::kResponseQueueMaxMessageSize;
    while (!done) {
      bool messageReceived = false;
      while (!messageReceived && !done) {
        messageReceived = mq->try_receive(message.data(), ipc::kResponseQueueMaxMessageSize, recvd_size, priority);
        std::this_thread::sleep_for(ipc::kQueuePollInterval);
      }
      if (done) continue;
      message[recvd_size] = 0;
      std::stringstream strstream;
      strstream << message.data();
      memset(message.data(), 0, ipc::kResponseQueueMaxMessageSize);
      ptree pt;
      read_info(strstream, pt);
      while (!spsc_queue_.push(pt))
        std::this_thread::sleep_for(ipc::kQueuePollInterval);
    }
  } catch (IPC::interprocess_exception &e) {
    SPDLOG_ERROR("IPC: {} (producer queue:{})", e.what(), std::string(ipc::kResponseQueuePrefix) + std::to_string(getpid()));
    done = true;
  }
}

ptree IpcClient::netClient(const std::string &netCommand, const std::string &netArgument) {
  ptree pt_response;
  ptree pt_request;
  try {
    IPC::managed_shared_memory mapSegment(IPC::open_only, std::string(ipc::kShmemSegment).c_str());
    IPC::named_mutex mapMutex(IPC::open_only, std::string(ipc::kMapMutex).c_str());
    pt_request.put("db.message", netCommand);
    pt_request.put("db.id", getpid());
    if (!netArgument.empty()) pt_request.put("db.argument", netArgument);

    IPC::message_queue mq(IPC::open_only, std::string(ipc::kQueryQueue).c_str());
    std::stringstream request_stream;
    write_info(request_stream, pt_request);
    mq.send(request_stream.str().c_str(), request_stream.str().length(), 0);

    std::pair<ipc::IPCMap *, std::size_t> ret = mapSegment.find<ipc::IPCMap>(std::string(ipc::kMapObject).c_str());
    ipc::IPCMap *mymap                        = ret.first;
    if (mymap == nullptr) {
      SPDLOG_ERROR("ipcClient: shared memory map '{}' not found", ipc::kMapObject);
      done = true;
      pt_response.put("error.response", "server not found");
      return pt_response;
    }

    std::size_t processId = getpid();
    auto it               = mymap->end();
    {
      IPC::scoped_lock<IPC::named_mutex> lock(mapMutex);
      it = mymap->find(processId);
    }

    // Budżet liczony ZEGAREM, a nie liczbą obrotów pętli. Obrót to sen plus
    // `lock` na muteksie współdzielonym z wątkiem emisji serwera, więc na
    // obciążonej maszynie trwa dłużej niż sam interwał — a wtedy liczenie prób
    // skracało faktyczne czekanie dokładnie w sytuacji, w której potrzebne było
    // najdłuższe (issue_217).
    const auto deadline = std::chrono::steady_clock::now() + clientResponseMaxFails_ * ipc::kClientResponsePollInterval;
    while (it == mymap->end() && std::chrono::steady_clock::now() < deadline) {
      std::this_thread::sleep_for(ipc::kClientResponsePollInterval);
      IPC::scoped_lock<IPC::named_mutex> lock(mapMutex);
      it = mymap->find(processId);
    }
    if (it == mymap->end()) {
      SPDLOG_ERROR("server not found");
      done = true;
      pt_response.put("error.response", "server not found");
      return pt_response;
    }
    std::stringstream strstream;
    {
      IPC::scoped_lock<IPC::named_mutex> lock(mapMutex);
      it = mymap->find(processId);
      if (it != mymap->end()) {
        strstream << it->second;
        mymap->erase(it);
      }
    }
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
