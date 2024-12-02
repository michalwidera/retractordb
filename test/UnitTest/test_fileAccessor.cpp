#include <gtest/gtest.h>

#include <rdb/fagrp.h>
#include <string>

TEST(xFileAccessor, test_dir) {
  ASSERT_TRUE(true);

  struct {
    char data;
  } record ;

  std::string filename = "/tmp/test_file";
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