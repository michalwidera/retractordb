#pragma once

#include <iostream>
#include <list>
#include <string>
#include <tuple>
#include <vector>

#include <boost/rational.hpp>

#include "field.hpp"
#include "rdb/descriptor.hpp"
#include "rdb/faccmemory.hpp"
#include "rule.hpp"
#include "token.hpp"

class qTree;

inline constexpr size_t kGeneratedPrefixLength = sizeof("STREAM_") - 1;

#ifdef RDB_COPY_COUNTER
// Diagnostyka inwestygacji speed_improvement: zliczanie kopii obiektu query.
// Sonda jest składnikiem query; jej KOPIUJĄCY ctor inkrementuje licznik, a
// PRZENOSZĄCY nie — dzięki temu niejawne składowe specjalne query pozostają
// niejawne (kopia query kopiuje sondę → zlicza; move query przenosi → nie).
// Cały blok znika bez śladu bez -DRDB_COPY_COUNTER=ON.
#include <atomic>
namespace qcopy {
extern std::atomic<long> queryCopyCount;
struct CopyProbe {
  CopyProbe() = default;
  CopyProbe(const CopyProbe &) { queryCopyCount.fetch_add(1, std::memory_order_relaxed); }
  CopyProbe &operator=(const CopyProbe &) {
    queryCopyCount.fetch_add(1, std::memory_order_relaxed);
    return *this;
  }
  CopyProbe(CopyProbe &&) noexcept            = default;
  CopyProbe &operator=(CopyProbe &&) noexcept = default;
};
}  // namespace qcopy
#endif

class query {
  void fillDescriptor(const std::list<field> &lSchemaVar, rdb::Descriptor &val, const std::string &id);

 public:
  explicit query(boost::rational<int> rInterval, std::string id);
  query();

  std::list<std::string> getFieldNamesList();

  std::string id;
  std::string filename;
  boost::rational<int> rInterval = 0;
  bool isDisposable              = false;
  bool isOneShot                 = false;
  bool isHold                    = false;
  bool isSubstrat                = false;

  std::list<field> lSchema;
  std::list<token> lProgram;

  std::list<rule> lRules;

  rdb::retention_t retention            = rdb::retention_t{.segments = 0, .capacity = 0};  // Retention segments and capacity
  std::pair<std::string, size_t> policy = std::make_pair("DEFAULT", rdb::memoryFile::no_retention);
  std::string storage_policy            = "DEFAULT";

#ifdef RDB_COPY_COUNTER
  [[no_unique_address]] qcopy::CopyProbe copyProbe_;  // diagnostyka: liczy kopie query (speed_improvement)
#endif

  [[nodiscard]] bool isDeclaration() const { return lProgram.empty(); }
  bool isReductionRequired();
  [[nodiscard]] bool isGenerated() const { return id.compare(0, kGeneratedPrefixLength, "STREAM_") == 0; }
  [[nodiscard]] bool isCompilerDirective() const { return !id.empty() && id[0] == ':'; }
  bool is(command_id command);

  std::vector<std::string> getDepStream();

  int getFieldIndex(const field &f);

  void reset();

  rdb::Descriptor descriptorStorage();
  rdb::Descriptor descriptorFrom(qTree &coreInstance);

  friend std::ostream &operator<<(std::ostream &os, const query &s);
};

bool operator<(const query &lhs, const query &rhs);

std::tuple<std::string, std::string, token> GetArgs(std::list<token> &prog);

bool isThere(const std::vector<query> &v, const std::string &query_name);
