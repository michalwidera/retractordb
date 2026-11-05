#include "ipcServer.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/info_parser.hpp>

#include "constants.hpp"
#include "ipcResponses.hpp"
#include "ipcSecurity.hpp"
#include "osPlatform.hpp"
#include "shmBudget.hpp"

namespace IPC = boost::interprocess;

namespace {

// Parser INFO traktuje linie, ktora po bialych znakach zaczyna sie od '#', jako dyrektywe, a
// '#include "plik"' otwiera dowolna sciezke z prawami serwera: plik zwykly zostaje dolaczony do
// komendy, FIFO blokuje watek komunikacyjny, a /dev/zero zjada pamiec. write_info takiej linii
// nie pisze (kazda zaczyna sie od klucza, a '\n' w wartosci idzie jako "\\n"), wiec komenda z nia
// nie pochodzi od klienta. Biale znaki jak is_ascii_space Boosta; '\n' konczy linie (getline).
bool hasInfoDirective(std::string_view text) {
  for (std::size_t lineStart = 0; lineStart < text.size();) {
    const auto first = text.find_first_not_of(" \t\v\f\r", lineStart);
    if (first == std::string_view::npos) return false;
    if (text[first] == '#') return true;
    const auto lineEnd = text.find('\n', first);
    if (lineEnd == std::string_view::npos) return false;
    lineStart = lineEnd + 1;
  }
  return false;
}

}  // namespace

void IpcServer::setServerName(std::string_view serverName) { names_ = ipc::names(serverName); }

void IpcServer::start(Callbacks callbacks) {
  callbacks_   = std::move(callbacks);
  commsThread_ = std::thread([this] {
    commandLoop();
    commsFinished_ = true;
  });
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
  //
  // Obcego watku tez nie dolaczamy bez limitu. FatalError w slocie przetwarzania biegnie pod
  // plan_epoch_mutex, a komenda wymagajaca modelu (`get`, `show`, `detail`, `adhoc`) czeka na
  // te blokade w commandProcessor. Watek, ktory jej nie dostanie nigdy, nie wraca do petli
  // i join() wisial do 2026-09-14 do SIGKILL. Watek wolny wychodzi z petli w jednym takcie
  // odpytywania, wiec limit dotyczy wylacznie watku zablokowanego -- ten ODPINAMY: stoi na
  // muteksie i do konca procesu niczego juz nie dotknie.
  if (commsThread_.joinable()) {
    if (commsThread_.get_id() == std::this_thread::get_id()) {
      commsThread_.detach();  // samego siebie nie da sie dolaczyc; ODPINAMY, bo destruktor
                              // std::thread nad watkiem dolaczalnym wola std::terminate
    } else {
      constexpr auto kExitJoinBudget = std::chrono::seconds(2);
      const auto deadline            = std::chrono::steady_clock::now() + kExitJoinBudget;
      while (!commsFinished_ && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(ipc::kQueuePollInterval);
      if (commsFinished_) {
        commsThread_.join();
      } else {
        SPDLOG_WARN("Communication thread did not stop within {} s of exit; detaching it.", kExitJoinBudget.count());
        commsThread_.detach();
      }
    }
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
  responsePool_.clear();  // zamknij mapowania puli przed unlink jej nazw w removeGlobalObjects()
  removeGlobalObjects();
  std::scoped_lock lock(clientMapsMutex_);
  removeClientQueues();
}

void IpcServer::removeGlobalObjects() const {
  ipc::security::removeOwned(names_.shmemSegment);
  ipc::security::removeOwned(names_.queryQueue);
  removePoolNames();
}

void IpcServer::removePoolNames() const {
  for (int index = 0; index < ipc::kResponseQueuePoolSegments; ++index)
    ipc::security::removeOwned(names_.responsePool(index));
}

/// Zamyka uchwyty i usuwa wlasne kolejki z zakonczonej epoki. Obcego UID nie
/// sprzatamy nawet po odmowie startu lub subskrypcji.
void IpcServer::removeClientQueues() {
  releasePendingLocked();
  // Najpierw zamkniecie mapowan, potem unlink -- ta sama kolejnosc co przy odroczonym zwolnieniu.
  id2QueueCache_.clear();
  for (const auto &element : id2StreamNameRelation_) {
    ipc::security::removeOwned(names_.responseQueue(element.first));
  }
  id2StreamNameRelation_.clear();
  subscriptionEpoch_.fetch_add(1, std::memory_order_release);
}

void IpcServer::releasePendingLocked() {
  for (auto &[clientId, queue] : pendingRelease_) {
    queue.reset();  // zamknij mapowanie przed unlink
    ipc::security::removeOwned(names_.responseQueue(clientId));
  }
  pendingRelease_.clear();
  releasePending_.store(false, std::memory_order_relaxed);
}

void IpcServer::releaseAbandonedQueues() {
  if (!releasePending_.load(std::memory_order_acquire)) return;
  decltype(pendingRelease_) released;
  {
    std::scoped_lock lock(clientMapsMutex_);
    released.swap(pendingRelease_);
    // Pojemnosc na kazdego obecnego subskrybenta: broadcast() dopisuje wtedy bez alokacji.
    pendingRelease_.reserve(id2StreamNameRelation_.size());
    releasePending_.store(false, std::memory_order_relaxed);
  }
  // Poza muteksem: munmap i unlink segmentu to milisekundy, a slot czekalby na nie przy
  // nastepnym broadcast(). Nazwa nalezy wciaz do zwalnianej kolejki - patrz pendingRelease_.
  for (auto &[clientId, queue] : released) {
    if (poolAbandonedQueue(clientId, queue)) {
      SPDLOG_WARN("queue pooled on timeout, procId={}", clientId);
      continue;
    }
    queue.reset();  // zamknij mapowanie przed unlink
    ipc::security::removeOwned(names_.responseQueue(clientId));
    SPDLOG_WARN("queue erased on timeout, procId={}", clientId);
  }
}

bool IpcServer::poolAbandonedQueue(int clientId, std::unique_ptr<IPC::message_queue> &queue) {
  // Zywy klient ma kolejke nadal zmapowana i po ponownym uzyciu czytalby wiersze nastepnego klienta.
  // Mapowanie wyklucza dopiero brak procesu potwierdzony przez jadro: zombie (watki grupy moga
  // jeszcze zyc) i nieczytelny wpis znacza "nie wiem", wiec kolejka idzie do zwolnienia jak dotad.
  const osplat::ProcessSnapshot owner = osplat::inspectProcess(clientId);
  if (owner.found || owner.unreadable) return false;
  const std::string queueName = names_.responseQueue(clientId);
  // Oproznienie przed zmiana nazwy: pod nazwa puli kolejka jest juz gotowa dla nastepnego klienta.
  if (!drainQueue(*queue, clientId)) return false;
  // Pelna pula ustepuje najswiezszej kolejce: koszt zwolnienia jest ten sam, a pula nie zostaje
  // na zawsze przy pojemnosciach strumieni, ktorych juz nikt nie subskrybuje.
  if (responsePool_.size() >= static_cast<std::size_t>(ipc::kResponseQueuePoolSegments)) {
    releaseOldestPooled();
  }
  int index = 0;
  while (std::ranges::any_of(responsePool_, [index](const PooledQueue &entry) { return entry.index == index; }))
    ++index;
  if (!ipc::security::renameOwned(queueName, names_.responsePool(index))) return false;
  responsePool_.push_back({.index = index, .queue = std::move(queue)});
  return true;
}

/// Odbior wszystkiego, co zostawil klient, przez uchwyt serwera: bez nowego mapowania. Samo wyzerowanie
/// licznika w naglowku nie wystarcza - Boost wymaga (BOOST_ASSERT w do_send), zeby wolny komunikat
/// mial wyzerowana dlugosc i priorytet, a zeruje je wlasnie odbior. Koszt rosnie z liczba
/// komunikatow: naglowek kazdego i kopia jego wiersza (#603, wariant D1a).
bool IpcServer::drainQueue(IPC::message_queue &queue, int clientId) {
  std::array<char, ipc::kResponseQueueMaxMessageSize> buffer;
  IPC::message_queue::size_type received = 0;
  unsigned int priority                  = 0;
  try {
    // Nikt juz do tej kolejki nie pisze (klient wypisany z rejestru), wiec wystarcza jeden przebieg
    // pojemnosci; licznik na koncu potwierdza, ze kolejka jest pusta.
    for (auto left = queue.get_max_msg(); left > 0 && queue.try_receive(buffer.data(), buffer.size(), received, priority);
         --left) {}
    return queue.get_num_msg() == 0;
  } catch (const IPC::interprocess_exception &ex) {
    // Takze not_recoverable: muteks porzucony przez zmarly proces - taka kolejka nie nadaje sie do puli.
    SPDLOG_WARN("response queue of client {} not drained for reuse: {}", clientId, ex.what());
    return false;
  }
}

void IpcServer::releaseOldestPooled() {
  responsePool_.front().queue.reset();  // zamknij mapowanie przed unlink
  ipc::security::removeOwned(names_.responsePool(responsePool_.front().index));
  responsePool_.erase(responsePool_.begin());
}

bool IpcServer::poolHas(int maxElements) const {
  return std::ranges::any_of(responsePool_, [maxElements](const PooledQueue &entry) {
    return entry.queue->get_max_msg() == static_cast<IPC::message_queue::size_type>(maxElements);
  });
}

std::unique_ptr<IPC::message_queue> IpcServer::takePooledQueue(int maxElements, const std::string &queueName) {
  // Dokladnie ta sama pojemnosc: zapas wolnego klienta (domyslnie 10 s) i wycena miejsca zostaja takie,
  // jakie dalaby nowa kolejka.
  const auto entry = std::ranges::find_if(responsePool_, [maxElements](const PooledQueue &candidate) {
    return candidate.queue->get_max_msg() == static_cast<IPC::message_queue::size_type>(maxElements);
  });
  if (entry == responsePool_.end()) return nullptr;
  std::unique_ptr<IPC::message_queue> queue;
  if (ipc::security::renameOwned(names_.responsePool(entry->index), queueName)) {
    queue = std::move(entry->queue);
  } else {
    entry->queue.reset();  // zamknij mapowanie przed unlink
    ipc::security::removeOwned(names_.responsePool(entry->index));
  }
  responsePool_.erase(entry);
  return queue;
}

bool IpcServer::subscribe(int clientId, const std::string &streamName, int maxElements, std::uint64_t expectedEpoch) {
  const std::string queueName = names_.responseQueue(clientId);

  // Straz miejsca PRZED utworzeniem kolejki. Kolejka boosta powstaje w calosci od razu (segment
  // jest wypelniany przy tworzeniu), wiec 10 s zapasu przy takcie 1 ms to jednorazowe 10 MiB --
  // a /dev/shm w kontenerze ma domyslnie 64 MiB. Bez tego sprawdzenia klient dostawal wylacznie
  // "No space left on device", czyli komunikat, ktory nie mowi ani ile brakuje, ani dlaczego
  // akurat ten strumien jest drogi. Pomiar nieudany (Space::known == false) PRZEPUSZCZA
  // subskrypcje: odmowa na podstawie niewiedzy byla by gorsza od proby.
  //
  // Kolejka z puli (#603) nie potrzebuje nowego miejsca. Bez niej bezczynna pula ustepuje nowej
  // kolejce, zanim padnie odmowa: jej zwolnienie kosztuje munmap i unlink, ale tylko przy braku miejsca.
  const bool pooled = poolHas(maxElements);
  if (!pooled) {
    const std::uint64_t needed = shmbudget::responseQueueBytes(maxElements);
    shmbudget::Space fs        = shmbudget::space();
    while (fs.known && fs.available < needed && !responsePool_.empty()) {
      releaseOldestPooled();
      fs = shmbudget::space();
    }
    if (fs.known && fs.available < needed)
      throw std::runtime_error(
          std::format("response queue for stream '{}' needs {} ({} elements of {} B), "
                      "shared memory has {} free",
                      streamName, shmbudget::humanBytes(needed), maxElements, ipc::kResponseQueueMaxMessageSize,
                      shmbudget::humanBytes(fs.available)));
  }

  // Rejestracja jest serializowana przez watek komend. Najpierw wypisujemy
  // stara subskrypcje, aby RT nie wysylal do odlinkowanej kolejki podczas mmap.
  // Obca kolejka pod ta nazwa daje odmowe zamiast usuniecia lub przejecia.
  //
  // Zalegle zwolnienie kolejki tego klienta (porzuconej w broadcast) przejmujemy pod TYM SAMYM
  // zajeciem muteksu (#600). Zwolnienie odroczone usuwa nazwe; gdyby zostalo na liscie, po
  // utworzeniu nowej kolejki o tej nazwie skasowaloby kolejke nowego klienta. Nazwe usuwa
  // removeOwned ponizej, mapowanie zamyka taken.clear() -- juz poza muteksem.
  decltype(pendingRelease_) taken;
  {
    std::scoped_lock lock(clientMapsMutex_);
    if (subscriptionEpoch_.load(std::memory_order_relaxed) != expectedEpoch) return false;
    if (!ipc::security::removeOwned(queueName))
      throw ipc::security::Rejected(std::format("response queue '{}' could not be safely removed", queueName));
    for (auto it = pendingRelease_.begin(); it != pendingRelease_.end();) {
      if (it->first == clientId) {
        taken.push_back(std::move(*it));
        it = pendingRelease_.erase(it);
      } else {
        ++it;
      }
    }
    id2QueueCache_.erase(clientId);
    id2StreamNameRelation_.erase(clientId);
  }
  taken.clear();
  // Pre-otwarcie kolejki TUTAJ, w watku komunikacyjnym (sledztwo ~40 ms,
  // JOURNAL.md 2026-07-18, Faza 3): tworzenie + mmap segmentu (~MB) nie moze
  // zostac w torze emisji watku RT -- lazy open_only w broadcast() kosztowal
  // 42 ms przy pierwszej emisji do nowego klienta (populacja stron mapowania
  // pod mlockall). Rejestracja w id2StreamNameRelation_ dopiero PO zbudowaniu
  // kolejki i razem z uchwytem pod muteksem, wiec watek RT nigdy nie widzi
  // klienta bez gotowego uchwytu. Tworzenie i mmap poza muteksem map oraz poza
  // blokada epoki planu (#283).
  //
  // Kolejka z puli jest juz zmapowana i pusta: zmiana nazwy zastepuje shm_open, fallocate i mmap.
  // Nieudana zmiana nazwy konczy sie zwyklym utworzeniem, juz bez strazy miejsca powyzej.
  auto queueHandle = pooled ? takePooledQueue(maxElements, queueName) : nullptr;
  if (!queueHandle) queueHandle = ipc::security::createQueue(queueName, maxElements, ipc::kResponseQueueMaxMessageSize);
  // Sprawdzenie epoki i wpis do map pod JEDNYM zajeciem muteksu: broadcastOutOfBusiness bierze
  // ten sam muteks, wiec albo rejestracja wypada przed nim i OOB ja obejmie, albo po nim i widzi
  // nowa epoke. Rejestr zamkniety od odczytu expectedEpoch oznacza, ze strumien, ktorego
  // parametry wolajacy odczytal, nalezy do epoki juz rozebranej.
  {
    std::scoped_lock lock(clientMapsMutex_);
    if (subscriptionEpoch_.load(std::memory_order_relaxed) == expectedEpoch) {
      id2StreamNameRelation_[clientId] = streamName;
      id2QueueCache_[clientId]         = std::move(queueHandle);
      // Miejsce na porzucenie kazdej zarejestrowanej kolejki: broadcast() dopisuje bez alokacji.
      pendingRelease_.reserve(pendingRelease_.size() + id2StreamNameRelation_.size());
      return true;
    }
  }
  // Do puli nie wraca: klient otwiera kolejke, gdy tylko pojawi sie jego nazwa, wiec mogl ja juz zmapowac.
  queueHandle.reset();  // zamknij mapowanie przed unlink -- ta sama kolejnosc co w releaseAbandonedQueues()
  ipc::security::removeOwned(queueName);
  return false;
}

/// Kolejka boosta trzyma w segmencie wlasny muteks, na Linuksie robust. Proces zabity z tym
/// muteksem w reku - klient w srodku try_receive - zostawia go NIENAPRAWIALNYM: kazde nastepne
/// zajecie rzuca lock_exception(not_recoverable), na zawsze. Wyjatek z try_send wychodzil poza
/// broadcast() az za petle przetwarzania, czyli jeden klient zabity w zlej chwili konczyl caly
/// serwer. Taka kolejka nalezy do martwego klienta, wiec traktujemy ja jak kolejke przepelniona:
/// falsz, a wolajacy ja kasuje i wypisuje klienta.
bool IpcServer::trySendOrAbandoned(IPC::message_queue &queue, const std::string &row, int clientId) {
  try {
    return queue.try_send(row.c_str(), row.length(), 0);
  } catch (const IPC::interprocess_exception &ex) {
    if (ex.get_error_code() != IPC::not_recoverable) throw;
    SPDLOG_ERROR("response queue of client {} abandoned by a process that died holding its lock", clientId);
    return false;
  }
}

// Wolajacy MUSI trzymac clientMapsMutex_ (dostep do oversizedRowStreams_).
bool IpcServer::rowFitsSlot(const std::string &row, std::string_view streamName) {
  if (row.length() <= static_cast<std::size_t>(ipc::kResponseQueueMaxMessageSize)) return true;
  if (oversizedRowStreams_.emplace(streamName).second)
    SPDLOG_ERROR(
        "stream '{}': serialized row is {} B and does not fit the {} B response queue slot; "
        "subscribers of this stream receive no data",
        streamName, row.length(), ipc::kResponseQueueMaxMessageSize);
  return false;
}

void IpcServer::broadcast(std::span<const std::string_view> streams, const RowFormatter &formatRow) {
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
    bool rowFits      = true;
    std::list<int> eraseList;
    for (const auto &element : id2StreamNameRelation_) {
      if (element.second == queryName) {
        if (!rowFormatted) {
          // RowFormatter bierze std::string, bo printRowValue szuka strumienia w mapie modelu po
          // nazwie. Konwersja wypada WYLACZNIE tutaj, czyli tylko dla strumienia, ktory ma
          // subskrybenta - obok budowanego w niej ptree jest niewidoczna.
          row          = formatRow(std::string(queryName));
          rowFormatted = true;
          rowFits      = rowFitsSlot(row, queryName);
        }
        // Wiersz dluzszy od slotu kolejki KONCZYL USLUGE. try_send rzuca wtedy
        // interprocess_exception, a najblizszy catch stoi poza petla przetwarzania
        // (executorsm.cpp), wiec pojedynczy subskrybent jednego szerokiego strumienia
        // zabieral serwer wszystkim pozostalym -- kod wyjscia no_child_process. Wiersz
        // niesie teraz jedna wartosc na ELEMENT, a nie na pole, wiec zwykle INTEGER[200]
        // wystarczy, zeby przekroczyc slot. Pomijamy taki wiersz i emitujemy dalej.
        if (!rowFits) break;
        //
        // Query discovery. queues are created by show command
        //
        const std::string queueName = names_.responseQueue(element.first);
        // Uchwyt kolejki z cache -- otwarcie (shm_open+mmap) tylko przy pierwszej
        // emisji do danego klienta, nie w kazdym slocie (patrz komentarz przy
        // id2QueueCache_).
        auto &mqPtr = id2QueueCache_[element.first];
        if (!mqPtr) mqPtr = ipc::security::openQueue(queueName);
        //
        // If send queue is full - means no one is listening and queue is
        // going to remove
        //
        // Bez munmap i unlink w slocie (#600): uchwyt idzie na liste, kolejke zwalnia
        // watek komunikacyjny w releaseAbandonedQueues().
        if (!trySendOrAbandoned(*mqPtr, row, element.first)) {
          pendingRelease_.emplace_back(element.first, std::move(mqPtr));
          releasePending_.store(true, std::memory_order_release);
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
    }
  }
}

void IpcServer::broadcastOutOfBusiness() {
  std::scoped_lock lock(clientMapsMutex_);
  for (const auto &element : id2StreamNameRelation_) {
    //
    // Uzywamy uchwytu wlasnej kolejki zamiast ponownie otwierac nazwe.
    // Porzucony muteks nadal moze rzucic wyjatek.
    //
    const std::string queueName = names_.responseQueue(element.first);
    try {
      auto &mq = *id2QueueCache_.at(element.first);
      //
      // Sending out-of-bussiness message
      //
      ptree pt;
      pt.put("stream", constants::Reserved_id_oob);
      std::stringstream strstream;
      write_info(strstream, pt);
      std::string row = strstream.str();

      (void)trySendOrAbandoned(mq, row, element.first);
    } catch (const std::exception &e) {
      SPDLOG_WARN("broadcastOutOfBusiness: end marker not sent to queue {}, procId={}: {}", queueName, element.first, e.what());
    }
    // Poza try: wyjatek wysylki nie moze ominac usuniecia - po wyczyszczeniu rejestru nikt
    // juz tej kolejki nie zna, a sweepAbandonedResources kolejek klientow nie rusza (#523).
    ipc::security::removeOwned(queueName);
    SPDLOG_WARN("queue erased on out-of-business, procId={}", element.first);
  }
  // Koniec epoki biegnie poza slotem, wiec zalegle zwolnienia wykonujemy tu synchronicznie.
  releasePendingLocked();
  id2StreamNameRelation_.clear();
  id2QueueCache_.clear();
  subscriptionEpoch_.fetch_add(1, std::memory_order_release);
}

// Procedura watku komunikacyjnego.
void IpcServer::commandLoop() {
  bool readyAnnounced = false;
  try {
    // Kasowanie na wejsciu sprzata po poprzedniku, ktory PADL: po SIGKILL segment i kolejka
    // zostaja w /dev/shm, a segment odpowiedzi moze nosic sloty w stanie posrednim. Jest to
    // bezpieczne wylacznie dlatego, ze
    // executorsm::run() przejmuje blokade instancji PRZED start() -- flock dowodzi, ze zaden
    // inny ZYWY serwer tych obiektow nie uzywa. Nazwy pochodza z names_, wiec kasowanie nigdy
    // nie siega poza obszar tego serwera.
    if (!ipc::security::removeOwned(names_.queryQueue) || !ipc::security::removeOwned(names_.shmemSegment))
      throw ipc::security::Rejected("IPC startup refused: existing resources could not be safely removed");
    // Pula poprzednika (#603). Obcy wpis pod nazwa puli nie blokuje startu: renameOwned na nia odmowi,
    // a porzucona kolejka zostanie wtedy zwolniona jak bez puli.
    removePoolNames();
    const ipc::responses::Mapping responses(IPC::create_only, names_.shmemSegment);
    const auto createCommandQueue = [this] {
      return ipc::security::createQueue(names_.queryQueue, ipc::kQueryQueueMaxMessages, ipc::kQueryQueueMaxMessageSize);
    };
    auto mq = createCommandQueue();
    callbacks_.onReady();
    readyAnnounced = true;
    //
    // This need to be clean up - There are some mess.
    //
    // try_receive moze oddac DOKLADNIE kQueryQueueMaxMessageSize bajtow (to max_message_size
    // kolejki), a message[recvd_size] = 0 pisze zaraz za nimi. Bez miejsca na terminator
    // komenda o pelnej dlugosci pisala jeden bajt poza tablica, w ramke stosu tego watku.
    std::array<char, ipc::kQueryQueueMaxMessageSize + ipc::kNullTerminatorBytes> message;
    static_assert(sizeof(message) > ipc::kQueryQueueMaxMessageSize, "bufor odbiorczy bez miejsca na terminator");
    unsigned int priority;
    IPC::message_queue::size_type recvd_size;

    // Klient zabity w srodku send z wewnetrznym muteksem kolejki w reku zostawia go
    // nienaprawialnym (patrz trySendOrAbandoned): kazde nastepne try_receive rzucaloby
    // lock_exception(not_recoverable), a wyjatek konczyl watek komunikacyjny - serwer liczyl
    // dalej, ale nie przyjmowal juz zadnej komendy, takze 'kill'. Kolejke zakladamy wtedy od
    // nowa pod ta sama nazwa. Komendy, ktore w niej czekaly, przepadaja: ich klienci koncza sie
    // limitem czasu, a kazdy nastepny otwiera juz nowa kolejke.
    const auto tryReceive = [&] {
      try {
        return mq->try_receive(message.data(), ipc::kQueryQueueMaxMessageSize, recvd_size, priority);
      } catch (const IPC::interprocess_exception &ex) {
        if (ex.get_error_code() != IPC::not_recoverable) throw;
        SPDLOG_ERROR("command queue '{}' abandoned by a process that died holding its lock; recreating it", names_.queryQueue);
        mq.reset();
        if (!ipc::security::removeOwned(names_.queryQueue))
          throw ipc::security::Rejected("command queue recovery refused: unsafe existing object");
        mq = createCommandQueue();
        return false;
      }
    };

    bool loopRunning = true;
    while (loopRunning) {
      while (tryReceive()) {
        message[recvd_size]      = 0;
        const bool infoDirective = hasInfoDirective(message.data());
        std::stringstream strstream;
        strstream << message.data();
        memset(message.data(), 0, message.size());
        // Blad jednej wiadomosci to ODMOWA tej wiadomosci, nie koniec watku. Tresc pisze dowolny
        // lokalny proces, a wyjatek poza tym blokiem konczy watek komunikacyjny: po onReady nikt
        // tego nie zauwaza (ipcFailed czyta sie tylko przy starcie), wiec serwer liczy dalej,
        // ale nie przyjmuje juz zadnej komendy, takze 'kill' i '--reset'. onCommand jest
        // oslonione w commandProcessor; ten blok obejmuje to, co wokol niego: parser INFO,
        // db.id oraz serializacje odpowiedzi.
        try {
          // Dyrektywa odrzucona PRZED parserem: read_info wykonalby ja, zanim cokolwiek sprawdzimy.
          if (infoDirective) {
            SPDLOG_ERROR("IPC command rejected: INFO directive ('#' line) in command text");
            continue;
          }
          ptree pt;
          read_info(strstream, pt);
          // db.id sprawdzane PRZED wykonaniem: komenda bez adresu zwrotnego jest odrzucana bez
          // skutkow ubocznych, zamiast wykonac sie (np. 'kill') i zgubic odpowiedz.
          int clientProcessId = 0;
          if (!boost::conversion::try_lexical_convert(pt.get("db.id", ""), clientProcessId)) {
            SPDLOG_ERROR("IPC command '{}' rejected: missing or invalid db.id, no client to answer", pt.get("db.message", ""));
            continue;
          }
          // db.seq odroznia kolejne zadania jednego procesu. Klient bez niego (starsza binarka)
          // dostaje seq 0 i nadal odbiera swoja odpowiedz, bo sam tez czeka na 0.
          std::uint64_t clientSeq = 0;
          boost::conversion::try_lexical_convert(pt.get("db.seq", "0"), clientSeq);
          // db.start to znacznik startu, ktory klient sam odczytal ze swojego wpisu procesu - wlasny
          // wpis jest czytelny zawsze. Serwer czytal go z wpisu klienta: przy hidepid=2 albo w innej
          // przestrzeni PID dostawal 0 i klient odrzucal wlasna odpowiedz, a po smierci klienta
          // i ponownym uzyciu jego PID-u przypisywal odpowiedz nowemu procesowi. Klient bez db.start
          // (starsza binarka) dostaje znacznik odczytany przez serwer, jak dotad.
          std::uint64_t clientStart = 0;
          boost::conversion::try_lexical_convert(pt.get("db.start", "0"), clientStart);
          ptree pt_retval;
          try {
            pt_retval = callbacks_.onCommand(pt);
          } catch (const std::exception &ex) {
            // Handler komendy ma wlasny catch(std::exception) i sam opisuje awarie klientowi;
            // tutaj trafia tylko to, co wyleci POZA nim. Odpowiedz jest obowiazkowa: klient czeka
            // na nia i bez niej melduje brak odpowiedzi, czyli wskazuje winnego po niewlasciwej
            // stronie IPC. Usluga biegnie dalej -- jedno wadliwe zadanie nie jest powodem, zeby
            // przestac obslugiwac pozostale.
            SPDLOG_CRITICAL("Command handler escaped its own boundary: {}", ex.what());
            pt_retval.clear();
            pt_retval.put("error.response", std::string("command handler failure: ") + ex.what());
          }
          // Sygnal idzie PO obsludze komendy, nie po jej odebraniu. Jedynym jego odbiorca jest
          // bramka --xqrywait, a bramka zdjeta w chwili ODEBRANIA komendy wpuszczala watek
          // przetwarzania jeszcze przed rejestracja subskrybenta: dla 'show' znaczylo to slot
          // wyemitowany do klienta, ktorego kolejki odpowiedzi jeszcze nie ma, a takiego wiersza
          // nikt juz nie odzyska. Rejestracja w subscribe() szla wtedy pod blokada epoki -- ta
          // sama, ktora bierze slot -- wiec wyscig rozstrzygal sie na ZAJECIU tej blokady:
          // wygrywal ten, kto siegnal po nia pierwszy. Przesuniecie sygnalu zamienia ten wyscig
          // na porzadek: gdy bramka opada, subskrypcja jest juz w mapach IpcServer. Od #283
          // subscribe() biegnie bez blokady epoki, a porzadek trzyma sie nadal, bo rejestracja
          // konczy sie przed powrotem z onCommand.
          // Kontrakt bramki jest nietkniety -- podnosi ja nadal KAZDA komenda, takze 'hello'
          // (patrz it_xqrywait_gate), takze komenda, ktora sie wywrocila. Wiadomosc odrzucona wyzej
          // komenda nie jest i jej nie podnosi.
          callbacks_.onCommandHandled();
          // Sending answer
          std::stringstream response_stream;
          write_info(response_stream, pt_retval);
          // Po znaczniku startu serwer rozpozna, ze wlasciciel slotu umarl, nawet jesli jego PID
          // dostal juz inny proces.
          if (clientStart == 0) clientStart = osplat::inspectProcess(clientProcessId).startTime;
          const ipc::responses::Owner owner{.pid = clientProcessId, .startTime = clientStart, .seq = clientSeq};
          ipc::responses::publish(responses.segment(), owner, response_stream.str());
        } catch (const std::exception &ex) {
          SPDLOG_ERROR("IPC message dropped: {}", ex.what());
        }
      }
      releaseAbandonedQueues();
      std::this_thread::sleep_for(ipc::kQueuePollInterval);

      if (callbacks_.shouldStop()) loopRunning = false;
    }
  } catch (const std::exception &ex) {
    // Kazda ucieczka z funkcji watku to std::terminate, stad std::exception, a nie tylko wyjatki
    // Boost IPC. onReady i onFailure wykluczaja sie (ipcServer.hpp), wiec po gotowosci zostaje
    // wylacznie glosny wpis: watek konczy prace, a serwer przestaje przyjmowac komendy.
    if (readyAnnounced) {
      SPDLOG_ERROR("IPC communication thread stopped, server no longer accepts commands: {}", ex.what());
      return;
    }
    // Najczestsza przyczyna to brak miejsca w /dev/shm: sama kolejka komend zajmuje 1000 KiB,
    // a w kontenerze caly tmpfs ma domyslnie 64 MiB. Zawiadomienie jest tu obowiazkowe --
    // wolajacy czeka na gotowosc IPC i bez niego stanie na zawsze.
    SPDLOG_ERROR("IPC resources could not be created: {}", ex.what());
    std::cerr << "Exception on server." << '\n' << ex.what() << '\n';
    if (callbacks_.onFailure) callbacks_.onFailure();
  }
}
