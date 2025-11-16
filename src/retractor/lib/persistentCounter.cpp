#include "persistentCounter.h"
#include <fstream>
persistentCounter::persistentCounter() : count_(0) {
    load();
}
persistentCounter::~persistentCounter() {
    save();
}
int persistentCounter::getCount() const {
    return count_;
}
void persistentCounter::increment() {
    ++count_;
}
void persistentCounter::load() {
    std::ifstream infile(filename_);
    if (infile.is_open()) {
        infile >> count_;
        infile.close();
    } else {
        count_ = 0; // Default to 0 if file doesn't exist (first use!)
      }
}

void persistentCounter::save() const {
    std::ofstream outfile(filename_);
    if (outfile.is_open()) {
        outfile << count_;
        outfile.close();
    }
}
