#pragma once
#include <atomic>
#include <string>

#include <boost/lockfree/spsc_queue.hpp>
#include <boost/property_tree/ptree.hpp>

class IpcTransport {
  boost::lockfree::spsc_queue<boost::property_tree::ptree, boost::lockfree::capacity<1024>> spsc_queue_;

 public:
  std::atomic<bool> done{false};

  boost::property_tree::ptree netClient(const std::string& cmd, const std::string& arg);
  void producer();
  bool popQueue(boost::property_tree::ptree& pt);
};
