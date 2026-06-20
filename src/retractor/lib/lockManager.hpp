#pragma once

#include <string>

class FlockServiceGuard {
 private:
  std::string lockFilePath;
  int lockFileDescriptor{-1};
  bool isLocked{false};

 public:
  explicit FlockServiceGuard(const std::string &serviceName);
  ~FlockServiceGuard();

  bool acquireLock();
  void setLockDir(const std::string &dir);
  [[nodiscard]] bool isLockActive() const;
  void releaseLock();
  [[nodiscard]] bool isAnotherInstanceRunning() const;

 private:
  [[nodiscard]] bool writeLockInfo() const;
  void cleanupLockFile();
};
