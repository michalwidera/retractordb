#include "persistentCounter.h"

#include <filesystem>
#include <fstream>

PersistentCounter::PersistentCounter() : count_(0) { load(); }
PersistentCounter::~PersistentCounter() {
  increment();
  save();
}
int PersistentCounter::getCount() const { return count_; }
void PersistentCounter::increment() { ++count_; }
void PersistentCounter::load() {
  std::ifstream infile(filename_);
  if (infile.is_open()) {
    infile >> count_;
    infile.close();
  } else {
    count_ = 0;  // Default to 0 if file doesn't exist (first use!)
  }
}

void PersistentCounter::save() {
  std::ofstream outfile(filename_);
  if (outfile.is_open()) {
    outfile << count_;
    outfile.close();
  }
}
