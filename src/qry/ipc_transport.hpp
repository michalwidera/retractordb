#pragma once
#include <atomic>
#include <string>

#include <boost/lockfree/spsc_queue.hpp>
#include <boost/property_tree/ptree.hpp>

// Pojemność wewnętrznej kolejki SPSC między wątkiem producenta (IPC) a wątkiem select().
// Wartość musi być potęgą 2 i wystarczająco duża by zaabsorbować burstowe dane strumieniowe.
constexpr int kSpscQueueCapacity = 1024;
constexpr int kIpcTransportDefaultClientResponseMaxFails = 10;

class IpcTransport {
  boost::lockfree::spsc_queue<boost::property_tree::ptree, boost::lockfree::capacity<kSpscQueueCapacity>> spsc_queue_;
  int clientResponseMaxFails_{kIpcTransportDefaultClientResponseMaxFails};

 public:
  std::atomic<bool> done{false};

  explicit IpcTransport(int clientResponseMaxFails = kIpcTransportDefaultClientResponseMaxFails);

  boost::property_tree::ptree netClient(const std::string &netCommand, const std::string &netArgument);
  void producer();
  bool popQueue(boost::property_tree::ptree &pt);
};
