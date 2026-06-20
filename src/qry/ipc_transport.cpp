#include "ipc_transport.hpp"

#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <print>
#include <sstream>
#include <string>
#include <thread>

#include <spdlog/spdlog.h>
#include <boost/container/map.hpp>
#include <boost/container/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/system/system_error.hpp>

#include "constants.hpp"

using boost::property_tree::ptree;
namespace IPC = boost::interprocess;

IpcTransport::IpcTransport(int clientResponseMaxFails)
  : clientResponseMaxFails_(std::max(1, clientResponseMaxFails)) {}

bool IpcTransport::popQueue(ptree &pt) { return spsc_queue_.pop(pt); }

void IpcTransport::producer() {
  try {
    std::string queueName = std::string(ipc::kResponseQueuePrefix) + std::to_string(getpid());
    IPC::message_queue mq(IPC::open_only, queueName.c_str());
    std::array<char, ipc::kResponseQueueMaxMessageSize> message;
    unsigned int priority{0};
    IPC::message_queue::size_type recvd_size = ipc::kResponseQueueMaxMessageSize;
    while (!done) {
      bool messageReceived = false;
      while (!messageReceived && !done) {
        messageReceived = mq.try_receive(message.data(), ipc::kResponseQueueMaxMessageSize, recvd_size, priority);
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

ptree IpcTransport::netClient(const std::string &netCommand, const std::string &netArgument) {
  ptree pt_response;
  ptree pt_request;
  try {
    using segment_manager_t = IPC::managed_shared_memory::segment_manager;
    using CharAllocator     = IPC::allocator<char, segment_manager_t>;
    using IPCString         = boost::container::basic_string<char, std::char_traits<char>, CharAllocator>;
    using KeyType           = int;
    using ValueType         = std::pair<const int, IPCString>;
    using ShmemAllocator    = IPC::allocator<ValueType, segment_manager_t>;
    using IPCMap            = boost::container::map<KeyType, IPCString, std::less<>, ShmemAllocator>;

    IPC::managed_shared_memory mapSegment(IPC::open_only, std::string(ipc::kShmemSegment).c_str());
    const ShmemAllocator allocatorShmemMapInstance(mapSegment.get_segment_manager());
    IPC::named_mutex mapMutex(IPC::open_only, std::string(ipc::kMapMutex).c_str());
    pt_request.put("db.message", netCommand);
    pt_request.put("db.id", getpid());
    if (!netArgument.empty()) pt_request.put("db.argument", netArgument);

    IPC::message_queue mq(IPC::open_only, std::string(ipc::kQueryQueue).c_str());
    std::stringstream request_stream;
    write_info(request_stream, pt_request);
    mq.send(request_stream.str().c_str(), request_stream.str().length(), 0);

    std::pair<IPCMap *, std::size_t> ret = mapSegment.find<IPCMap>(std::string(ipc::kMapObject).c_str());
    IPCMap *mymap                        = ret.first;
    if (mymap == nullptr) {
      SPDLOG_ERROR("ipc_transport: shared memory map '{}' not found", ipc::kMapObject);
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

    int cntr(clientResponseMaxFails_);
    while (it == mymap->end() && cntr != 0) {
      std::this_thread::sleep_for(ipc::kClientResponsePollInterval);
      IPC::scoped_lock<IPC::named_mutex> lock(mapMutex);
      it = mymap->find(processId);
      --cntr;
    }
    if (it == mymap->end()) {
      std::println("server not found");
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
