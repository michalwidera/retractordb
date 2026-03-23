#include "lockManager.hpp"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <iostream>

FlockServiceGuard::FlockServiceGuard(const std::string &serviceName)
    : lockFileDescriptor(-1),
      isLocked(false),
      lockFilePath("") {
  lockFilePath = std::filesystem::temp_directory_path() / (serviceName + ".lock");
  SPDLOG_INFO("Service guard for {} initialized", serviceName);
  SPDLOG_INFO("Lock file path: {}", lockFilePath);
}

FlockServiceGuard::~FlockServiceGuard() { releaseLock(); }

bool FlockServiceGuard::acquireLock() {
  SPDLOG_INFO("Attempting to acquire lock...");

  // Open or create the lock file
  lockFileDescriptor = open(lockFilePath.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (lockFileDescriptor == -1) {
    SPDLOG_ERROR("Failed to open lock file: {} ,errno: {}", lockFilePath, strerror(errno));
    return false;
  }

  SPDLOG_INFO("Lock file opened (FD: {})", lockFileDescriptor);

  // Non-blocking exclusive lock
  int flockResult = flock(lockFileDescriptor, LOCK_EX | LOCK_NB);

  if (flockResult == -1) {
    std::cerr << "Another instance is running, errno: " << strerror(errno) << std::endl;
    if (errno == EWOULDBLOCK || errno == EAGAIN) {
      SPDLOG_WARN("Other instance is already running, cannot acquire lock on: {}", lockFilePath);
    } else {
      SPDLOG_ERROR("Unexpected error while locking: {} , errno: {}", lockFilePath, strerror(errno));
    }

    close(lockFileDescriptor);
    lockFileDescriptor = -1;
    return false;
  }

  SPDLOG_INFO("Lock acquired on file: {}", lockFilePath);
  isLocked = true;

  if (!writeLockInfo()) {
    SPDLOG_WARN("Cannot write process info to lock file: {}, bypass.", lockFilePath);
  }

  return true;
}

bool FlockServiceGuard::isLockActive() const { return isLocked && lockFileDescriptor != -1; }

void FlockServiceGuard::releaseLock() {
  if (isLocked && lockFileDescriptor != -1) {
    SPDLOG_INFO("Releasing lock on file: {}", lockFilePath);

    if (flock(lockFileDescriptor, LOCK_UN) == -1) {
      SPDLOG_WARN("Failed to release lock: {} , errno: {}", lockFilePath, strerror(errno));
    }

    if (close(lockFileDescriptor) == -1) {
      SPDLOG_WARN("Failed to close lock file descriptor: {} , errno: {}", lockFilePath, strerror(errno));
    }

    lockFileDescriptor = -1;
    isLocked           = false;

    cleanupLockFile();

    SPDLOG_INFO("Lock released and cleaned up: {}", lockFilePath);
  }
}

bool FlockServiceGuard::isAnotherInstanceRunning() const {
  int testFd = open(lockFilePath.c_str(), O_RDONLY);
  if (testFd == -1) {
    return false;
  }

  int result     = flock(testFd, LOCK_SH | LOCK_NB);
  bool isRunning = (result == -1 && (errno == EWOULDBLOCK || errno == EAGAIN));

  close(testFd);
  return isRunning;
}

bool FlockServiceGuard::writeLockInfo() {
  if (lockFileDescriptor == -1) {
    return false;
  }

  // Clear the file first
  if (ftruncate(lockFileDescriptor, 0) == -1) {
    return false;
  }

  // Move to the beginning of the file
  if (lseek(lockFileDescriptor, 0, SEEK_SET) == -1) {
    return false;
  }

  // Process information
  std::string processInfo = "PID: " + std::to_string(getpid()) + "\n";
  processInfo += "PPID: " + std::to_string(getppid()) + "\n";

  // Write to the file
  ssize_t written = write(lockFileDescriptor, processInfo.c_str(), processInfo.length());
  if (written == -1) {
    return false;
  }

  // Ensure data is written to disk
  if (fsync(lockFileDescriptor) == -1) {
    return false;
  }

  SPDLOG_INFO("Process info written to lock file: {}", lockFilePath);
  return true;
}

void FlockServiceGuard::cleanupLockFile() {
  if (unlink(lockFilePath.c_str()) == 0) {
    SPDLOG_INFO("Lock file removed: {}", lockFilePath);
  }
}
