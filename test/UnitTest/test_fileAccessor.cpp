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

const std::filesystem::path sandBoxFolder = "/tmp/test_fileAccessor";

std::ifstream::pos_type filesize(std::string filename) {
  std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}

std::vector<BYTE> readFile(std::string filename) {
  // open the file:
  std::ifstream file(filename, std::ios::binary);

  // read the data:
  return std::vector<BYTE>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

TEST(xFileAccessor, test_dir) {
  bool filepathExists = std::filesystem::is_directory(sandBoxFolder);
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

  ASSERT_TRUE(true);

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
  auto retention   = std::pair<int, int>(silos_count, silos_size);
  auto gfa         = std::make_unique<rdb::groupFileAccessor<uint8_t>>(filename, recsize, retention);
  record.data      = 11;
  gfa->write(reinterpret_cast<uint8_t *>(&record));
  record.data = 12;
  gfa->write(reinterpret_cast<uint8_t *>(&record));

  for (const auto &entry : std::filesystem::directory_iterator(sandBoxFolder))
    mapOfFiles[entry.path()] = fileInfo{filesize(entry.path()), readFile(entry.path())};

  for (const auto &entry : mapOfFiles) {
    std::cout << entry.first << ":" << entry.second.sizeFromSystem;
    for (auto &d : entry.second.fileContents) {
      std::cout << ":" << (int)d;
    }
    std::cout << std::endl;
  }
}