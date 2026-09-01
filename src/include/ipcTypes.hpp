#pragma once

#include <functional>
#include <string>
#include <utility>

#include <boost/container/map.hpp>
#include <boost/container/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

// Layout mapy odpowiedzi w pamieci dzielonej.
//
// Ten sam naglowek obowiazuje serwer (ipcServer) i klienta (ipcClient).
// Wczesniej kazda strona deklarowala te aliasy u siebie -- rozjazd
// ktoregokolwiek z nich nie jest bledem kompilacji, tylko niezgodnym
// odczytem segmentu, wiec definicja moze byc tylko jedna.
namespace ipc {

using segment_manager_t = boost::interprocess::managed_shared_memory::segment_manager;

using CharAllocator = boost::interprocess::allocator<char, segment_manager_t>;
using IPCString     = boost::container::basic_string<char, std::char_traits<char>, CharAllocator>;

using ValueType = std::pair<const int, IPCString>;

using ShmemAllocator = boost::interprocess::allocator<ValueType, segment_manager_t>;
using IPCMap         = boost::container::map<int, IPCString, std::less<>, ShmemAllocator>;

}  // namespace ipc
