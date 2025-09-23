#pragma once

#include <string>

class FlockServiceGuard {
 private:
  std::string lockFilePath;
  int lockFileDescriptor;
  bool isLocked;

 public:
  FlockServiceGuard(const std::string &serviceName, const std::string &lockDir = "/tmp");
  ~FlockServiceGuard();

  bool acquireLock();
  bool isLockActive() const;
  void releaseLock();
  bool isAnotherInstanceRunning() const;

 private:
  bool writeLockInfo();
  void cleanupLockFile();
};
