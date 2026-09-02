#include "ipcServer.hpp"

#include <array>
#include <cstring>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#include <spdlog/spdlog.h>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/info_parser.hpp>

#include "constants.hpp"
#include "ipcTypes.hpp"

namespace IPC = boost::interprocess;

using ipc::IPCMap;
using ipc::IPCString;
using ipc::ShmemAllocator;

void IpcServer::setServerName(std::string_view serverName) { names_ = ipc::names(serverName); }

void IpcServer::start(Callbacks callbacks) {
  callbacks_   = std::move(callbacks);
  commsThread_ = std::thread([this] { commandLoop(); });
}

void IpcServer::stop() {
  if (commsThread_.joinable()) commsThread_.join();
}

std::thread::native_handle_type IpcServer::threadHandle() { return commsThread_.native_handle(); }

void IpcServer::shutdownFromExitHandler() {
  // Nie dolaczamy watku, ktory WLASNIE wykonuje to sprzatanie. FatalError konczy proces
  // przez std::exit, a ten uruchamia funkcje atexit W WATKU, ktory go wywolal -- takze
  // w watku komunikacyjnym: commandLoop -> commandProcessor -> getAdHoc -> compile(),
  // a kompilator ma wiele wywolan FatalError. join() na watku biezacym rzuca
  // std::system_error("Resource deadlock avoided"), a wyjatek z handlera atexit to
  // std::terminate: JEDNO wadliwe zapytanie ad hoc (np. `src@(0,4)`) zabijalo serwer
  // SIGABRT-em zamiast zakonczyc go z EXIT_FAILURE, i to juz po wypisaniu wlasciwej
  // diagnostyki. Watek i tak konczy sie razem z procesem, wiec pominiecie join() niczego
  // nie zostawia w locie; sprzatanie IPC ponizej wykonuje sie wtedy normalnie.
  if (commsThread_.joinable()) {
    if (commsThread_.get_id() == std::this_thread::get_id())
      commsThread_.detach();  // samego siebie nie da sie dolaczyc; ODPINAMY, bo destruktor
                              // std::thread nad watkiem dolaczalnym wola std::terminate
    else
      commsThread_.join();
  }
  removeGlobalObjects();
  // Kolejki klientow tylko pod try_lock. Handler atexit biegnie ROWNOLEGLE do watku
  // przetwarzajacego, ktory moze trzymac clientMapsMutex_ przez caly przebieg
  // broadcast(); bezwarunkowe lock() zamienialoby wyciek kolejki w zawieszenie
  // procesu przy wyjsciu, czyli lekarstwo gorsze od choroby. Nieodebrany muteks
  // znaczy tyle, ze kolejki zostaja -- tak jak zostawaly zawsze przed ta zmiana.
  std::unique_lock lock(clientMapsMutex_, std::try_to_lock);
  if (lock.owns_lock()) removeClientQueues();
}

void IpcServer::removeAllObjects() {
  removeGlobalObjects();
  std::scoped_lock lock(clientMapsMutex_);
  removeClientQueues();
}

void IpcServer::removeGlobalObjects() {
  IPC::shared_memory_object::remove(names_.shmemSegment.c_str());
  IPC::message_queue::remove(names_.queryQueue.c_str());
  IPC::named_mutex::remove(names_.mapMutex.c_str());
}

/// Kolejka odpowiedzi, ktora przetrwa smierc serwera, nie jest tylko smieciem w
/// /dev/shm: subscribe() otwiera ja przez open_or_create, a ten przy ISTNIEJACEJ
/// kolejce IGNORUJE zadana pojemnosc. Klient, ktory po restarcie dostanie ten sam
/// PID, dostaje kolejke o glebokosci z poprzedniego przebiegu i przepelnia ja
/// w miejscu, ktorego nie da sie powiazac z przyczyna. Dlatego kasuje je KAZDA
/// droga wyjscia, nie tylko normalna.
void IpcServer::removeClientQueues() {
  // Najpierw zamkniecie mapowan, potem unlink -- ta sama kolejnosc co w broadcast().
  id2QueueCache_.clear();
  for (const auto &element : id2StreamNameRelation_) {
    IPC::message_queue::remove(names_.responseQueue(element.first).c_str());
  }
  id2StreamNameRelation_.clear();
}

void IpcServer::subscribe(int clientId, const std::string &streamName, int maxElements) {
  const std::string queueName = names_.responseQueue(clientId);
  // Pre-otwarcie kolejki TUTAJ, w watku komunikacyjnym (sledztwo ~40 ms,
  // JOURNAL.md 2026-07-18, Faza 3): tworzenie + mmap segmentu (~MB) nie moze
  // zostac w torze emisji watku RT -- lazy open_only w broadcast() kosztowal
  // 42 ms przy pierwszej emisji do nowego klienta (populacja stron mapowania
  // pod mlockall). Rejestracja w id2StreamNameRelation_ dopiero PO zbudowaniu
  // kolejki i razem z uchwytem pod muteksem, wiec watek RT nigdy nie widzi
  // klienta bez gotowego uchwytu (przy okazji znika dotychczasowy wyscig
  // rejestracja-przed-utworzeniem-kolejki). Nadpisanie uchwytu przy
  // re-rejestracji zamyka stare mapowanie w tym watku.
  auto queueHandle = std::make_unique<IPC::message_queue>(IPC::open_or_create,               // open or create
                                                          queueName.c_str(),                 // name
                                                          maxElements,                       // max message number
                                                          ipc::kResponseQueueMaxMessageSize  // max message size
  );
  {
    std::scoped_lock lock(clientMapsMutex_);
    id2StreamNameRelation_[clientId] = streamName;
    id2QueueCache_[clientId]         = std::move(queueHandle);
  }
}

void IpcServer::broadcast(const std::set<std::string> &streams, const RowFormatter &formatRow) {
  // Muteks na caly przebieg emisji: kontencja tylko z krotkim wstawieniem do map
  // przy rejestracji klienta (watek komunikacyjny trzyma go nanosekundy), a koszt
  // niekontendowanego lock/unlock raz na slot jest pomijalny wobec ~ms compute.
  std::scoped_lock lock(clientMapsMutex_);
  for (const auto &queryName : streams) {
    // Formatowanie wiersza (printRowValue: ptree + serializacja wszystkich pol)
    // jest kosztowne i potrzebne wylacznie subskrybentom -- wykonuj leniwie,
    // dopiero przy pierwszym kliencie danego strumienia. Bez subskrybentow
    // wiersz i tak byl wyrzucany (petla ponizej nie robila nic).
    std::string row;
    bool rowFormatted = false;
    std::list<int> eraseList;
    for (const auto &element : id2StreamNameRelation_) {
      if (element.second == queryName) {
        if (!rowFormatted) {
          row          = formatRow(queryName);
          rowFormatted = true;
        }
        //
        // Query discovery. queues are created by show command
        //
        const std::string queueName = names_.responseQueue(element.first);
        // Uchwyt kolejki z cache -- otwarcie (shm_open+mmap) tylko przy pierwszej
        // emisji do danego klienta, nie w kazdym slocie (patrz komentarz przy
        // id2QueueCache_).
        auto &mqPtr = id2QueueCache_[element.first];
        if (!mqPtr) mqPtr = std::make_unique<IPC::message_queue>(IPC::open_only, queueName.c_str());
        //
        // If send queue is full - means no one is listening and queue is
        // going to remove
        //
        if (!mqPtr->try_send(row.c_str(), row.length(), 0)) {
          mqPtr.reset();  // zamknij mapowanie przed unlink
          IPC::message_queue::remove(queueName.c_str());
          eraseList.push_back(element.first);
        }
      }
    }
    //
    // cleaning form clients map that are not receiving data from queue
    //
    for (const auto &element : eraseList) {
      id2StreamNameRelation_.erase(element);
      id2QueueCache_.erase(element);
      SPDLOG_WARN("queue erased on timeout, procId={}", element);
    }
  }
}

void IpcServer::broadcastOutOfBusiness() {
  std::scoped_lock lock(clientMapsMutex_);
  for (const auto &element : id2StreamNameRelation_) {
    //
    // Queue may have been removed earlier (try_send overflow in broadcast).
    // Wrap in try-catch to avoid crash on open_only failure.
    //
    const std::string queueName = names_.responseQueue(element.first);
    try {
      IPC::message_queue mq(IPC::open_only, queueName.c_str());
      //
      // Sending out-of-bussiness message
      //
      ptree pt;
      pt.put("stream", constants::Reserved_id_oob);
      std::stringstream strstream;
      write_info(strstream, pt);
      std::string row = strstream.str();

      mq.try_send(row.c_str(), row.length(), 0);
      IPC::message_queue::remove(queueName.c_str());
      SPDLOG_WARN("queue erased on out-of-business, procId={}", element.first);
    } catch (IPC::interprocess_exception &e) {
      SPDLOG_WARN("broadcastOutOfBusiness: queue {} already removed, procId={}: {}", queueName, element.first, e.what());
    }
  }
  id2StreamNameRelation_.clear();
  id2QueueCache_.clear();
}

// Procedura watku komunikacyjnego.
void IpcServer::commandLoop() {
  try {
    // Kasowanie na wejsciu sprzata po poprzedniku, ktory PADL: po SIGKILL segment, kolejka
    // i muteks zostaja w /dev/shm, a open_or_create trafiloby na nie i probowalo skonstruowac
    // mape w segmencie, w ktorym ona juz jest. Jest to bezpieczne wylacznie dlatego, ze
    // executorsm::run() przejmuje blokade instancji PRZED start() -- flock dowodzi, ze zaden
    // inny ZYWY serwer tych obiektow nie uzywa. Nazwy pochodza z names_, wiec kasowanie nigdy
    // nie siega poza obszar tego serwera.
    IPC::message_queue::remove(names_.queryQueue.c_str());
    IPC::shared_memory_object::remove(names_.shmemSegment.c_str());
    IPC::named_mutex::remove(names_.mapMutex.c_str());
    // Segment and allocator for map purposes
    IPC::managed_shared_memory mapSegment(IPC::open_or_create, names_.shmemSegment.c_str(), ipc::kShmemSegmentSize);
    const ShmemAllocator allocatorShmemMapInstance(mapSegment.get_segment_manager());
    IPC::named_mutex mapMutex(IPC::open_or_create, names_.mapMutex.c_str());
    // Create a message_queue.
    IPC::message_queue mq(IPC::open_or_create,            // open or crate
                          names_.queryQueue.c_str(),      // name
                          ipc::kQueryQueueMaxMessages,    // max message number
                          ipc::kQueryQueueMaxMessageSize  // max message size
    );
    IPCMap *mymap = mapSegment.construct<IPCMap>(std::string(ipc::kMapObject).c_str())  // object name
                    (std::less<>(), allocatorShmemMapInstance);
    callbacks_.onReady();
    //
    // This need to be clean up - There are some mess.
    //
    std::array<char, ipc::kQueryQueueMaxMessageSize> message;
    unsigned int priority;
    IPC::message_queue::size_type recvd_size;

    bool loopRunning = true;
    while (loopRunning) {
      while (mq.try_receive(message.data(), ipc::kQueryQueueMaxMessageSize, recvd_size, priority)) {
        callbacks_.onMessageReceived();

        message[recvd_size] = 0;
        std::stringstream strstream;
        strstream << message.data();
        memset(message.data(), 0, ipc::kQueryQueueMaxMessageSize);
        ptree pt;
        read_info(strstream, pt);
        ptree pt_retval     = callbacks_.onCommand(pt);
        int clientProcessId = boost::lexical_cast<int>(pt.get("db.id", ""));
        // Sending answer
        std::stringstream response_stream;
        write_info(response_stream, pt_retval);
        IPCString ipcResponse(allocatorShmemMapInstance);
        ipcResponse = response_stream.str().c_str();
        // cppcheck-suppress danglingTemporaryLifetime
        {
          IPC::scoped_lock<IPC::named_mutex> lock(mapMutex);
          mymap->insert(std::pair<int, IPCString>(clientProcessId, ipcResponse));
        }
      }
      std::this_thread::sleep_for(ipc::kQueuePollInterval);

      if (callbacks_.shouldStop()) loopRunning = false;
    }
  } catch (IPC::interprocess_exception &ex) {
    std::cout << "Exception on server." << '\n' << ex.what() << '\n';
  }
}
