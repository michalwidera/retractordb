#include <gtest/gtest.h>
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
    std::cerr << "Filesystem error: " << e.what() << '\n';
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

  auto recsize     = sizeof(BYTE);
  auto silos_count = 0;
  auto silos_size  = 0;
  auto retention   = rdb::retention_t{silos_count, silos_size};
  auto gfa         = std::make_unique<rdb::groupFileAccessor<uint8_t>>(filename, recsize, retention);
  record.data      = 11;
  gfa->write(reinterpret_cast<uint8_t *>(&record));
  record.data = 12;
  gfa->write(reinterpret_cast<uint8_t *>(&record));

  for (const auto &entry : std::filesystem::directory_iterator(sandBoxFolder)) {
    mapOfFiles[entry.path().string()] = fileInfo(filesize(entry.path().string()), readFile(entry.path().string()));
  }

  for (const auto &entry : mapOfFiles) {
    std::stringstream ss;
    ss << entry.first << ":" << entry.second.sizeFromSystem;
    for (auto &d : entry.second.fileContents) {
      ss << ":" << static_cast<int>(d);
    }
    GTEST_LOG_(INFO) << ss.str();
  }
}