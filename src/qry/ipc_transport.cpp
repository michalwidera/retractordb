#include "ipc_transport.hpp"

#include <unistd.h>

#include <array>
#include <chrono>
#include <cstring>
#include <sstream>
#include <thread>

#include <spdlog/spdlog.h>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/system/system_error.hpp>

using boost::property_tree::ptree;
namespace IPC = boost::interprocess;

bool IpcTransport::popQueue(ptree &pt) { return spsc_queue_.pop(pt); }

void IpcTransport::producer() {
  try {
    const int messageSize = 1024;
    std::string queueName = "brcdbr" + std::to_string(getpid());
    IPC::message_queue mq(IPC::open_only, queueName.c_str());
    std::array<char, messageSize> message;
    unsigned int priority{0};
    IPC::message_queue::size_type recvd_size = messageSize;
    while (!done) {
      bool messageReceived = false;
      while (!messageReceived && !done) {
        messageReceived = mq.try_receive(message.data(), messageSize, recvd_size, priority);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
      if (done) continue;
      message[recvd_size] = 0;
      std::stringstream strstream;
      strstream << message.data();
      memset(message.data(), 0, 1000);
      ptree pt;
      read_info(strstream, pt);
      while (!spsc_queue_.push(pt))
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  } catch (IPC::interprocess_exception &e) {
    SPDLOG_ERROR("IPC: {} (producer queue:{})", e.what(), "brcdbr" + std::to_string(getpid()));
    done = true;
  }
}

ptree IpcTransport::netClient(const std::string &netCommand, const std::string &netArgument) {
  ptree pt_response;
  ptree pt_request;
  try {
    typedef IPC::managed_shared_memory::segment_manager segment_manager_t;
    typedef IPC::allocator<char, segment_manager_t> CharAllocator;
    typedef IPC::basic_string<char, std::char_traits<char>, CharAllocator> IPCString;
    typedef int KeyType;
    typedef std::pair<const int, IPCString> ValueType;
    typedef IPC::allocator<ValueType, segment_manager_t> ShmemAllocator;
    typedef IPC::map<KeyType, IPCString, std::less<KeyType>, ShmemAllocator> IPCMap;

    IPC::managed_shared_memory mapSegment(IPC::open_only, "RetractorShmemMap");
    const ShmemAllocator allocatorShmemMapInstance(mapSegment.get_segment_manager());
    IPC::named_mutex mapMutex(IPC::open_only, "RetractorMapMutex");
    pt_request.put("db.message", netCommand);
    pt_request.put("db.id", getpid());
    if (netArgument != "") pt_request.put("db.argument", netArgument);

    IPC::message_queue mq(IPC::open_only, "RetractorQueryQueue");
    std::stringstream request_stream;
    write_info(request_stream, pt_request);
    mq.send(request_stream.str().c_str(), request_stream.str().length(), 0);

    std::pair<IPCMap *, std::size_t> ret = mapSegment.find<IPCMap>("MyMap");
    IPCMap *mymap                        = ret.first;
    if (!mymap) {
      SPDLOG_ERROR("ipc_transport: shared memory map 'MyMap' not found");
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

    // When server works under valgrind - must be 10 probes x 10ms
    constexpr int maxAcceptableFails = 10;
    int cntr(maxAcceptableFails);
    while (it == mymap->end() && cntr) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      IPC::scoped_lock<IPC::named_mutex> lock(mapMutex);
      it = mymap->find(processId);
      --cntr;
    }
    if (it == mymap->end()) {
      printf("server not found\n");
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
