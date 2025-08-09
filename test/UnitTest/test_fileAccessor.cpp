#include <gtest/gtest.h>
#include <rdb/faccmemory.h>
#include <rdb/fagrp.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

typedef unsigned char BYTE;

// cd build/Debug
// make install ; ctest -R test_fileAccessor -V

// Source under test:
// /home/michal/GitHub/retractordb/src/rdb/lib/fagrp.cc
// /home/michal/GitHub/retractordb/src/include/rdb/fagrp.h

const std::filesystem::path sandBoxFolder = "/tmp/test_fileAccessor";

std::ifstream::pos_type filesize(const std::string &filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

std::vector<BYTE> readFile(const std::string &filename) {
  // open the file:
  std::ifstream file(filename, std::ios::binary);

  // read the data:
  return std::vector<BYTE>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

TEST(FileAccessorTest, test_dir) {
  bool filepathExists = false;
  try {
    filepathExists = std::filesystem::is_directory(sandBoxFolder);
  } catch (const std::filesystem::filesystem_error &e) {
    GTEST_LOG_(FATAL) << "Filesystem error: " << e.what() << '\n';
  }
  if (filepathExists) {
    std::filesystem::remove_all(sandBoxFolder);
  }

  std::filesystem::create_directories(sandBoxFolder);
  std::filesystem::permissions(              //
      sandBoxFolder,                         //
      std::filesystem::perms::others_all,    //
      std::filesystem::perm_options::remove  //
  );
  std::filesystem::current_path(sandBoxFolder);

  struct {
    BYTE data;
  } record;

  struct fileInfo {
    int sizeFromSystem;
    std::vector<BYTE> fileContents;
  };

  std::map<std::string, fileInfo> mapOfFiles;

  std::string filename = "test_file";

  auto recsize                = sizeof(BYTE);
  rdb::segments_t silos_count = 0;
  rdb::capacity_t silos_size  = 0;
  auto retention              = rdb::retention_t{silos_count, silos_size};
  auto gfa                    = std::make_unique<rdb::groupFileAccessor>(filename, recsize, retention);
  record.data                 = 11;
  gfa->write(reinterpret_cast<uint8_t *>(&record));
  record.data = 12;
  gfa->write(reinterpret_cast<uint8_t *>(&record));

  for (const auto &entry : std::filesystem::directory_iterator(sandBoxFolder)) {
    mapOfFiles[entry.path().string()] = fileInfo(filesize(entry.path().string()), readFile(entry.path().string()));
  }

  for (const auto &entry : mapOfFiles) {
    GTEST_LOG_(INFO) << "filename:" << entry.first;
    GTEST_LOG_(INFO) << "filesize:" << entry.second.sizeFromSystem;
    std::stringstream ss;
    for (auto &d : entry.second.fileContents) ss << "[" << static_cast<int>(d) << "]";
    GTEST_LOG_(INFO) << "contents:" << ss.str();
  }

  GTEST_ASSERT_EQ(mapOfFiles.size(), 1);
  GTEST_ASSERT_EQ(mapOfFiles["/tmp/test_fileAccessor/test_file"].sizeFromSystem, 2);
  GTEST_ASSERT_EQ(mapOfFiles["/tmp/test_fileAccessor/test_file"].fileContents, std::vector<BYTE>({11, 12}));
  GTEST_ASSERT_EQ(gfa->count(), 2);  // count is not affected by retention
}

TEST(MemoryAccessorTest, test_infinite_memory_accessor) {
  struct {
    BYTE data;
  } record;

  std::string filename = "test_file_memory";

  auto recsize   = sizeof(BYTE);
  auto retention = rdb::memoryFileAccessor::no_retention;
  auto mfa       = std::make_unique<rdb::memoryFileAccessor>(filename, recsize, retention);

  // Write records
  record.data = 1;
  GTEST_ASSERT_EQ(mfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);
  record.data = 2;
  GTEST_ASSERT_EQ(mfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);
  record.data = 3;
  GTEST_ASSERT_EQ(mfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);
  record.data = 4;
  GTEST_ASSERT_EQ(mfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(mfa->count(), 4);

  GTEST_ASSERT_EQ(mfa->read(reinterpret_cast<uint8_t *>(&record), 0), EXIT_SUCCESS);
  GTEST_LOG_(INFO) << "Read record data: " << static_cast<int>(record.data);
  GTEST_ASSERT_EQ(record.data, 1);

  GTEST_ASSERT_EQ(mfa->read(reinterpret_cast<uint8_t *>(&record), 1), EXIT_SUCCESS);
  GTEST_LOG_(INFO) << "Read record data: " << static_cast<int>(record.data);
  GTEST_ASSERT_EQ(record.data, 2);

  GTEST_ASSERT_EQ(mfa->read(reinterpret_cast<uint8_t *>(&record), 2), EXIT_SUCCESS);
  GTEST_LOG_(INFO) << "Read record data: " << static_cast<int>(record.data);
  GTEST_ASSERT_EQ(record.data, 3);

  GTEST_ASSERT_EQ(mfa->read(reinterpret_cast<uint8_t *>(&record), 3), EXIT_SUCCESS);
  GTEST_LOG_(INFO) << "Read record data: " << static_cast<int>(record.data);
  GTEST_ASSERT_EQ(record.data, 4);

  mfa->write(nullptr);  // Clear the storage
  GTEST_ASSERT_EQ(mfa->count(), 0);  // After clearing, count should be 0
}

TEST(MemoryAccessorTest, test_retention_memory_accessor) {
  struct {
    BYTE data;
  } record;

  std::string filename = "test_file_memory";

  auto recsize   = sizeof(BYTE);
  auto retention = 2;
  auto mfa       = std::make_unique<rdb::memoryFileAccessor>(filename, recsize, retention);

  // Write records
  record.data = 1;
  GTEST_ASSERT_EQ(mfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);
  record.data = 2;
  GTEST_ASSERT_EQ(mfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);
  record.data = 3;
  GTEST_ASSERT_EQ(mfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);
  record.data = 4;
  GTEST_ASSERT_EQ(mfa->write(reinterpret_cast<uint8_t *>(&record)), EXIT_SUCCESS);

  GTEST_ASSERT_EQ(mfa->count(), 4);

  GTEST_ASSERT_EQ(mfa->read(reinterpret_cast<uint8_t *>(&record), 2), EXIT_SUCCESS);
  GTEST_LOG_(INFO) << "Read record data: " << static_cast<int>(record.data);
  GTEST_ASSERT_EQ(record.data, 3);

  GTEST_ASSERT_EQ(mfa->read(reinterpret_cast<uint8_t *>(&record), 3), EXIT_SUCCESS);
  GTEST_LOG_(INFO) << "Read record data: " << static_cast<int>(record.data);
  GTEST_ASSERT_EQ(record.data, 4);

  GTEST_LOG_(INFO) << "Reading record at index 0 - expect failure";
  GTEST_ASSERT_EQ(mfa->read(reinterpret_cast<uint8_t *>(&record), 0), EXIT_FAILURE);

  mfa->write(nullptr);  // Clear the storage
  GTEST_ASSERT_EQ(mfa->count(), 0);  // After clearing, count should be 0
}

TEST(FileAccessorTest, test_retention_one_read_and_retention) {
  bool filepathExists = false;
  try {
    filepathExists = std::filesystem::is_directory(sandBoxFolder);
  } catch (const std::filesystem::filesystem_error &e) {
    GTEST_LOG_(FATAL) << "Filesystem error: " << e.what() << '\n';
  }
  if (filepathExists) {
    std::filesystem::remove_all(sandBoxFolder);
  }

  std::filesystem::create_directories(sandBoxFolder);
  std::filesystem::permissions(              //
      sandBoxFolder,                         //
      std::filesystem::perms::others_all,    //
      std::filesystem::perm_options::remove  //
  );
  std::filesystem::current_path(sandBoxFolder);

  struct {
    BYTE data;
  } record;

  struct fileInfo {
    int sizeFromSystem;
    std::vector<BYTE> fileContents;
  };

  std::map<std::string, fileInfo> mapOfFiles;

  std::string filename = "test_file";

  auto recsize                = sizeof(BYTE);
  rdb::segments_t silos_count = 2;
  rdb::capacity_t silos_size  = 3;
  auto retention              = rdb::retention_t{silos_count, silos_size};
  auto gfa                    = std::make_unique<rdb::groupFileAccessor>(filename, recsize, retention);

  // Write records
  record.data = 1;
  gfa->write(reinterpret_cast<uint8_t *>(&record));
  record.data = 2;
  gfa->write(reinterpret_cast<uint8_t *>(&record));
  record.data = 3;
  gfa->write(reinterpret_cast<uint8_t *>(&record));
  record.data = 4;
  gfa->write(reinterpret_cast<uint8_t *>(&record));
  record.data = 5;
  gfa->write(reinterpret_cast<uint8_t *>(&record));
  record.data = 6;
  gfa->write(reinterpret_cast<uint8_t *>(&record));

  for (const auto &entry : std::filesystem::directory_iterator(sandBoxFolder)) {
    mapOfFiles[entry.path().string()] = fileInfo(filesize(entry.path().string()), readFile(entry.path().string()));
  }

  for (const auto &entry : mapOfFiles) {
    GTEST_LOG_(INFO) << "filename:" << entry.first;
    GTEST_LOG_(INFO) << "filesize:" << entry.second.sizeFromSystem;
    std::stringstream ss;
    for (auto &d : entry.second.fileContents) ss << "[" << static_cast<int>(d) << "]";
    GTEST_LOG_(INFO) << "contents:" << ss.str();
  }

  // Check file contents and sizes

  GTEST_ASSERT_EQ(mapOfFiles.size(), 2);
  GTEST_ASSERT_EQ(mapOfFiles["/tmp/test_fileAccessor/test_file_segment_0"].sizeFromSystem, 3);
  GTEST_ASSERT_EQ(mapOfFiles["/tmp/test_fileAccessor/test_file_segment_1"].sizeFromSystem, 3);
  GTEST_ASSERT_EQ(mapOfFiles["/tmp/test_fileAccessor/test_file_segment_0"].fileContents, std::vector<BYTE>({1, 2, 3}));
  GTEST_ASSERT_EQ(mapOfFiles["/tmp/test_fileAccessor/test_file_segment_1"].fileContents, std::vector<BYTE>({4, 5, 6}));
  GTEST_ASSERT_EQ(gfa->count(), 6);

  // Read records
  gfa->read(reinterpret_cast<uint8_t *>(&record), 0);
  GTEST_LOG_(INFO) << "Read record data: " << static_cast<int>(record.data);
  GTEST_ASSERT_EQ(record.data, 1);

  gfa->read(reinterpret_cast<uint8_t *>(&record), 1);
  GTEST_LOG_(INFO) << "Read record data: " << static_cast<int>(record.data);
  GTEST_ASSERT_EQ(record.data, 2);

  gfa->read(reinterpret_cast<uint8_t *>(&record), 2);
  GTEST_LOG_(INFO) << "Read record data: " << static_cast<int>(record.data);
  GTEST_ASSERT_EQ(record.data, 3);

  gfa->read(reinterpret_cast<uint8_t *>(&record), 3);
  GTEST_LOG_(INFO) << "Read record data: " << static_cast<int>(record.data);
  GTEST_ASSERT_EQ(record.data, 4);

  gfa->read(reinterpret_cast<uint8_t *>(&record), 4);
  GTEST_LOG_(INFO) << "Read record data: " << static_cast<int>(record.data);
  GTEST_ASSERT_EQ(record.data, 5);

  gfa->read(reinterpret_cast<uint8_t *>(&record), 5);
  GTEST_LOG_(INFO) << "Read record data: " << static_cast<int>(record.data);
  GTEST_ASSERT_EQ(record.data, 6);  // last record
}