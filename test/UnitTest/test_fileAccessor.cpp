#include <gtest/gtest.h>

#include <rdb/fagrp.h>
#include <string>

#include <filesystem>

const std::string sandBoxFolder = "/tmp/test_sandbox";

TEST(xFileAccessor, test_dir) {

  bool filepathExists = std::filesystem::is_directory(sandBoxFolder);

  if (!filepathExists) {
    std::filesystem::create_directories(sandBoxFolder);
    std::filesystem::permissions(
        sandBoxFolder,
        std::filesystem::perms::others_all,
        std::filesystem::perm_options::remove
    );
  }

  std::filesystem::current_path(sandBoxFolder);

  ASSERT_TRUE(true);

  struct {
    char data;
  } record ;

  const std::string filename = "test_file";
  auto recsize = sizeof(char);
  auto silos_count = 3;
  auto silos_size = 3;
  auto retention = std::pair<int, int>(silos_count,silos_size);
  auto gfa = std::make_unique<rdb::groupFileAccessor<uint8_t>>(filename,recsize,retention);
  record.data = 1;
  gfa->write(reinterpret_cast<uint8_t *>(&record));
  record.data = 2;
  gfa->write(reinterpret_cast<uint8_t *>(&record));
}