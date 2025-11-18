#pragma once
#include <string>

/* This is specific persistent counter dedicated for stream management.
   It stores it's value in file "rdb_percounter" in current working directory.
   Each time an object of this class is created it loads value from file,
   and on destruction
   It increases it's value after each destruction.
   On creation it loads value from file.
   On destruction it saves current value to file.
*/

class PersistentCounter {
 public:
  PersistentCounter();
  ~PersistentCounter();
  int getCount() const;
  constexpr static const char *persistentCounterFilename_ = "rdb.cnt";

 private:
  int count_;
  void load();
  void save();
  void increment();
};
