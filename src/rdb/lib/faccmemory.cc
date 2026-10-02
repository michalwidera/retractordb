#include "rdb/faccmemory.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>  // for std::copy
#include <cerrno>
#include <limits>
#include <ranges>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "rdb/exceptions.hpp"

namespace rdb {

// Faza 2 refaktoru: stan magazynu MEMORY - dawniej mapy `static` tego pliku - przeniosl sie do
// MemoryStore. Byl jedynym stanem magazynu MEMORY i byl stanem CALEGO PROCESU, wiec dwa silniki
// zbudowane w jednym procesie dzielily dane strumienia o tej samej nazwie, nic o sobie nie wiedzac.
// Kubelek jednego strumienia (memoryBucket) i jego zycie do ostatniej instancji - patrz memoryStore.hpp.

namespace {

/// Straz na jedyna droge, ktora uchwyt mogl by rozjechac sie z nazwa: `name()` zwraca
/// `std::string &` (tak stanowi FileInterface), wiec ktos MOGLBY zmienic nazwe instancji po
/// konstrukcji, a zapamietany wezel zostalby przy poprzedniej. Dzis tego nie robi nikt -
/// wszystkie wywolania `name()` w drzewie tylko czytaja - i ta asercja jest po to, zeby
/// ewentualna zmiana tego stanu rzeczy byla czerwonym testem, a nie cicha pomylka.
///
/// Tylko Debug: w Release kosztowalaby dokladnie to wyszukanie, ktore zapamietany wezel usuwa.
void assertBucketMatches([[maybe_unused]] const memoryBucket *cached, [[maybe_unused]] MemoryStore &store,
                         [[maybe_unused]] const std::string &filename) {
#ifndef NDEBUG
  if (cached != &store.bucket(filename))
    throw LogicError(
        fmt::format("memoryFile: zapamietany kubelek nie odpowiada nazwie '{}' - nazwa zmieniona po konstrukcji?", filename));
#endif
}

}  // namespace

memoryFile::memoryFile(const std::string_view fileName, const Descriptor &descriptor,
                       const std::pair<std::string, size_t> &retentionSize, MemoryStore &store)
    : filename_(std::string(fileName)),                                //
      recordSize_(static_cast<ssize_t>(descriptor.getSizeInBytes())),  //
      retentionSize_(retentionSize.second),                            //
      store_(store),                                                   //
      // Jedyne wyszukanie po nazwie w calym cyklu zycia tej instancji. Wezly std::map sa
      // stabilne - uniewaznia je wylacznie skasowanie tego elementu, a kasuje go dopiero
      // destruktor OSTATNIEJ instancji, ktora go uzywa (write(nullptr, ...) czysci wektory POD
      // kluczem, nie klucz). Poki ta instancja zyje, wezel jest wiec wazny. Strumien dolaczony
      // ad-hoc buduje wlasne memoryFile z wlasnym wyszukaniem i istniejacych instancji nie dotyka.
      bucket_(&store.bucket(filename_)) {
  // Kubelek z rekordami innego rozmiaru nie ma czego przekazac tej instancji: read() kopiuje rekord
  // w calosci, wiec szerszy wyszedlby poza bufor wolajacego. Nowy plan i tak dostaje swiezy kubelek
  // (poprzedni ginie z ostatnia instancja) - to jest obrona, nie droga glowna.
  if (bucket_->recordSize != recordSize_) {
    bucket_->data.clear();
    bucket_->nulls.clear();
    bucket_->writeCount = 0;
    bucket_->recordSize = recordSize_;
  }
  ++bucket_->users;
}

/// Kubelek znika razem z ostatnia instancja, ktora go uzywa. Do 2026-09-27 kubelki zyly do
/// konca procesu: po `xqry --reset` strumien o tej samej nazwie dziedziczyl rekordy i writeCount
/// poprzedniego planu (przesuniety pierscien, zapis poza bufor przy wezszym rekordzie, awaria
/// przy odczycie poza rozmiarem kubelka), a nazwy nieobecne w nowym planie zostawaly w pamieci.
memoryFile::~memoryFile() {
  if (--bucket_->users == 0) store_.erase(filename_);
}

auto memoryFile::name() -> std::string & { return filename_; }

ssize_t memoryFile::write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) {
  if (recordSize_ == 0) throw LogicError("memoryFile::write: recordSize_ is zero - accessor built on a zero-width descriptor");
  assertBucketMatches(bucket_, store_, filename_);
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
      // Kolowy bufor: zapisz do slotu writeCount % retentionSize_
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
    // Nadpisanie rekordu pod wskazana pozycja
    const size_t slot = (retentionSize_ != no_retention) ? (location % retentionSize_) : location;
    if (slot >= bucket.data.size()) {
      SPDLOG_ERROR("Write failed: slot {} out of range, storage size {}", slot, bucket.data.size());
      return ERANGE;
    }
    bucket.data[slot] = std::move(vec);
    if (slot < bucket.nulls.size()) bucket.nulls[slot] = nullBitset;
  }
  return EXIT_SUCCESS;
}

ssize_t memoryFile::read(uint8_t *ptrData, std::vector<bool> &nullBitset, const size_t position) {
  if (recordSize_ == 0) throw LogicError("memoryFile::read: recordSize_ is zero - accessor built on a zero-width descriptor");
  assertBucketMatches(bucket_, store_, filename_);
  auto &bucket        = *bucket_;
  const auto location = position / recordSize_;
  const size_t slot   = (retentionSize_ != no_retention) ? (location % retentionSize_) : location;

  if (slot >= bucket.data.size()) {
    SPDLOG_ERROR("Read failed: slot {} out of range, storage size {}", slot, bucket.data.size());
    return ERANGE;
  }

  // Rekord kopiujemy w calosci, wiec rekord innego rozmiaru niz ta instancja wyszedlby poza bufor
  // wolajacego (albo zostawil w nim smieci). Konstruktor czysci kubelek o innym rozmiarze, ale nie
  // pomoze, gdy obie instancje zyja naraz - dlatego straz stoi tutaj.
  if (std::cmp_not_equal(bucket.data[slot].size(), recordSize_)) {
    SPDLOG_ERROR("Read failed: record in slot {} has {} bytes, '{}' reads {}", slot, bucket.data[slot].size(), filename_,
                 recordSize_);
    return EINVAL;
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
  assertBucketMatches(bucket_, store_, filename_);
  return bucket_->writeCount;
}

size_t memoryFile::bucketCountForUnitTest() { return MemoryStore::processDefault().size(); }

}  // namespace rdb
