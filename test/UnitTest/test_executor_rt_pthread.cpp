#include <pthread.h>

#include <algorithm>
#include <cerrno>

#include <gtest/gtest.h>

#include "platformConfig.h"
#include "retractor/lib/executor_rt.hpp"

namespace {
int scheduleError = 0, scheduleCalls = 0, scheduledPolicy = -1, scheduledPriority = -1;
pthread_t scheduledThread{};

int recordScheduling(pthread_t thread, int policy, const struct sched_param *parameter) {
  ++scheduleCalls;
  scheduledThread   = thread;
  scheduledPolicy   = policy;
  scheduledPriority = parameter->sched_priority;
  return scheduleError;
}
}  // namespace

// Rzeczywista galaz pthread takze na Linuksie, bez uprawnien RT i zmiany polityki watku.
// Jak test_busFallback: zmieniamy mozliwosci tylko w tej jednostce testowej.
#undef RDB_HAS_SCHED_SETSCHEDULER
#define RDB_HAS_SCHED_SETSCHEDULER 0
#undef RDB_HAS_PTHREAD_SCHEDPARAM
#define RDB_HAS_PTHREAD_SCHEDPARAM 1
#undef RDB_HAS_MLOCKALL
#define RDB_HAS_MLOCKALL      0
#define pthread_setschedparam recordScheduling
#include "retractor/lib/executor_rt.cpp"
#undef pthread_setschedparam

TEST(ExecutorRtPthread, SuccessAndRefusalAgreeWithTheSchedulingCall) {
  // covers: executorrt.rt-activate.3~1, executorrt.rt-activate.4~1, executorrt.rt-activate.6~1
  for (const int error : {0, EPERM, EINVAL}) {
    for (const int priority : {0, 50, 1000}) {
      SCOPED_TRACE(error);
      SCOPED_TRACE(priority);
      scheduleError = error;
      scheduleCalls = 0;
      EXPECT_EQ(rtActivate(priority), error == 0);
      EXPECT_EQ(scheduleCalls, 1);
      EXPECT_TRUE(pthread_equal(scheduledThread, pthread_self()));
      EXPECT_EQ(scheduledPolicy, SCHED_FIFO);
      EXPECT_EQ(scheduledPriority, std::clamp(priority, sched_get_priority_min(SCHED_FIFO), sched_get_priority_max(SCHED_FIFO)));
    }
  }
}
