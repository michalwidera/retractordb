#include <string>

class persistentCounter {
 public:
  persistentCounter();
  ~persistentCounter();
  int getCount() const;
  void increment();

 private:
  const std::string filename_ = "rdb_counter.dat";
  int count_;
  void load();
  void save() const;
};
