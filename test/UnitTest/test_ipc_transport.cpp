#include <gtest/gtest.h>

#include <boost/property_tree/ptree.hpp>

#include "qry/ipc_transport.hpp"

// ---- Lifecycle ----

TEST(IpcTransport, done_starts_false) {
  IpcTransport t;
  EXPECT_FALSE(t.done);
}

TEST(IpcTransport, done_can_be_set_and_read) {
  IpcTransport t;
  t.done = true;
  EXPECT_TRUE(t.done);
}

TEST(IpcTransport, done_is_independently_per_instance) {
  IpcTransport a;
  IpcTransport b;
  a.done = true;
  EXPECT_TRUE(a.done);
  EXPECT_FALSE(b.done);
}

// ---- popQueue ----

TEST(IpcTransport, popQueue_returns_false_on_empty) {
  IpcTransport t;
  boost::property_tree::ptree pt;
  EXPECT_FALSE(t.popQueue(pt));
}

TEST(IpcTransport, popQueue_does_not_modify_pt_on_empty) {
  IpcTransport t;
  boost::property_tree::ptree pt;
  pt.put("key", "original");
  t.popQueue(pt);
  EXPECT_EQ(pt.get<std::string>("key"), "original");
}

TEST(IpcTransport, multiple_instances_have_independent_queues) {
  IpcTransport a;
  IpcTransport b;
  boost::property_tree::ptree pt;
  EXPECT_FALSE(a.popQueue(pt));
  EXPECT_FALSE(b.popQueue(pt));
}
