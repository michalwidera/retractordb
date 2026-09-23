#include "rdb/faccmemory.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>  // for std::copy
#include <limits>
#include <map>
#include <ranges>
#include <utility>
#include <vector>
#include "fatalError.hpp"

namespace rdb {

/// Stan JEDNEGO strumienia trzymanego w pamięci procesu: rekordy, ich bitsety null i licznik
/// zapisów - w jednym węźle mapy.
///
/// Do 2026-09-23 były to TRZY mapy kluczowane tą samą nazwą, a każdy dostęp do rekordu płacił
/// za wyszukanie w każdej z nich osobno (read: trzy, write: do czterech). Callgrind przypisywał
/// samym `map::operator[]` 3,75 % instrukcji slotu na planie ADD-owym i 4,86 % na potoku EKG,
/// nie licząc udziału w `__memcmp_avx2` na porównaniach kluczy. Jeden węzeł na strumień znosi
/// cały ten rachunek: wyszukanie jest JEDNO i zapada w konstruktorze memoryFile.
struct memoryBucket {
  std::vector<std::vector<uint8_t>> data;
  std::vector<std::vector<bool>> nulls;
  size_t writeCount{0};  // Licznik zapisów - logiczna pozycja niezależna od instancji.
};

}  // namespace rdb

/// Stan współdzielony po nazwie strumienia między instancjami memoryFile - ten sam kontrakt co
/// przedtem, tylko w jednym drzewie zamiast w trzech.
static std::map<std::string, rdb::memoryBucket> memoryStore;

namespace {

/// Straż na jedyną drogę, którą uchwyt mógłby rozjechać się z nazwą: `name()` zwraca
/// `std::string &` (tak stanowi FileInterface), więc ktoś MÓGŁBY zmienić nazwę instancji po
/// konstrukcji, a zapamiętany węzeł zostałby przy poprzedniej. Dziś tego nie robi nikt -
/// wszystkie wywołania `name()` w drzewie tylko czytają - i ta asercja jest po to, żeby
/// ewentualna zmiana tego stanu rzeczy była czerwonym testem, a nie cichą pomyłką.
///
/// Tylko Debug: w Release kosztowałaby dokładnie to wyszukanie, które ta zmiana usuwa.
void assertBucketMatches([[maybe_unused]] const rdb::memoryBucket *cached, [[maybe_unused]] const std::string &filename) {
#ifndef NDEBUG
  if (cached != &memoryStore[filename])
    FatalError("memoryFile: zapamiętany kubełek nie odpowiada nazwie '{}' - nazwa zmieniona po konstrukcji?", filename);
#endif
}

}  // namespace

namespace rdb {

memoryFile::memoryFile(const std::string_view fileName, const Descriptor &descriptor,
                       const std::pair<std::string, size_t> &retentionSize)
    : filename_(std::string(fileName)),                                //
      recordSize_(static_cast<ssize_t>(descriptor.getSizeInBytes())),  //
      retentionSize_(retentionSize.second),                            //
      // Jedyne wyszukanie po nazwie w całym cyklu życia tej instancji. Węzły std::map są
      // stabilne - unieważnia je wyłącznie skasowanie tego elementu, a z memoryStore nic nigdy
      // nie kasuje (write(nullptr, ...) czyści wektory POD kluczem, nie klucz). Strumień
      // dołączony ad-hoc buduje własne memoryFile z własnym wyszukaniem i istniejących
      // instancji nie dotyka.
      bucket_(&memoryStore[filename_]) {}

auto memoryFile::name() -> std::string & { return filename_; }

ssize_t memoryFile::write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) {
  if (recordSize_ == 0) FatalError("memoryFile::write: recordSize_ is zero");
  assertBucketMatches(bucket_, filename_);
  auto &bucket  = *bucket_;
  auto location = position / recordSize_;
  if (ptrData == nullptr) {
    bucket.data.clear();
    bucket.nulls.clear();
    bucket.writeCount = 0;
    return EXIT_SUCCESS;
  }

  std::vector<uint8_t> vec(ptrData, ptrData + recordSize_);

  if (position == std::numeric_limits<size_t>::max()) {
    if (retentionSize_ != no_retention) {
      // Kołowy bufor: zapisz do slotu writeCount % retentionSize_
      const size_t wc   = bucket.writeCount;
      const size_t slot = wc % retentionSize_;
      if (slot < bucket.data.size()) {
        bucket.data[slot]  = std::move(vec);
        bucket.nulls[slot] = nullBitset;
      } else {
        bucket.data.push_back(std::move(vec));
        bucket.nulls.push_back(nullBitset);
      }
    } else {
      bucket.data.push_back(std::move(vec));
      bucket.nulls.push_back(nullBitset);
    }
    bucket.writeCount++;
  } else {
    // Nadpisanie rekordu pod wskazaną pozycją
    const size_t slot = (retentionSize_ != no_retention) ? (location % retentionSize_) : location;
    if (slot >= bucket.data.size()) {
      SPDLOG_ERROR("Write failed: slot {} out of range, storage size {}", slot, bucket.data.size());
      return EXIT_FAILURE;
    }
    bucket.data[slot] = std::move(vec);
    if (slot < bucket.nulls.size()) bucket.nulls[slot] = nullBitset;
  }
  return EXIT_SUCCESS;
}

ssize_t memoryFile::read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) {
  if (recordSize_ == 0) FatalError("memoryFile::read: recordSize_ is zero");
  assertBucketMatches(bucket_, filename_);
  auto &bucket        = *bucket_;
  const auto location = position / recordSize_;
  const size_t slot   = (retentionSize_ != no_retention) ? (location % retentionSize_) : location;

  if (slot >= bucket.data.size()) {
    SPDLOG_ERROR("Read failed: slot {} out of range, storage size {}", slot, bucket.data.size());
    return EXIT_FAILURE;
  }

  std::ranges::copy(bucket.data[slot], ptrData);

  if (slot < bucket.nulls.size()) {
    nullBitset = bucket.nulls[slot];
  } else {
    nullBitset.clear();
  }
  return EXIT_SUCCESS;
}

size_t memoryFile::count() {
  assertBucketMatches(bucket_, filename_);
  return bucket_->writeCount;
}

}  // namespace rdb
