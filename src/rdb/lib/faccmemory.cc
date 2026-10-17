#include "rdb/faccmemory.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>  // for std::copy
#include <cerrno>
#include <limits>
#include <ranges>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "rdb/error.hpp"

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
/// Tylko Debug: w Release kosztowałaby dokładnie to wyszukanie, które ta zmiana usuwa.
void assertBucketMatches([[maybe_unused]] const rdb::memoryBucket *cached, [[maybe_unused]] rdb::MemoryStore &store,
                         [[maybe_unused]] const std::string &filename) {
#ifndef NDEBUG
  RDB_ASSERT(cached == &store.bucket(filename),
             "memoryFile: zapamietany kubelek nie odpowiada nazwie '{}' - nazwa zmieniona po konstrukcji?", filename);
#endif
}

/// Czy rekord `location` nadal lezy w pierscieniu. Przy retencji R pierscien trzyma wylacznie
/// ostatnie R zapisow, czyli rekordy [writeCount - R, writeCount); slot `location % R` rekordu
/// starszego albo jeszcze niezapisanego nalezy do INNEGO rekordu. Do 2026-10-16 read() oddawal
/// wtedy ten cudzy rekord z sukcesem, a update w miejscu go nadpisywal - po cichu, inaczej niz
/// magazyn dyskowy (groupFile::read zwraca ERANGE dla usunietego segmentu). Bez retencji granice
/// sprawdza dalej warunek `slot >= bucket.data.size()`.
bool retained(const rdb::memoryBucket &bucket, const size_t location, const size_t retentionSize) {
  if (retentionSize == rdb::memoryFile::no_retention) return true;
  return location < bucket.writeCount && bucket.writeCount - location <= retentionSize;
}

}  // namespace

memoryFile::memoryFile(const std::string_view fileName, const Descriptor &descriptor,
                       const std::pair<std::string, size_t> &retentionSize, MemoryStore &store)
    : filename_(std::string(fileName)),                                //
      recordSize_(static_cast<ssize_t>(descriptor.getSizeInBytes())),  //
      retentionSize_(retentionSize.second),                            //
      // Jedyne wyszukanie po nazwie w całym cyklu życia tej instancji. Węzły std::map są
      // stabilne - unieważnia je wyłącznie skasowanie tego elementu, a kasuje go dopiero
      // destruktor OSTATNIEJ instancji, która go używa (write(nullptr, ...) czyści wektory POD
      // kluczem, nie klucz). Póki ta instancja żyje, węzeł jest więc ważny. Strumień dołączony
      // ad-hoc buduje własne memoryFile z własnym wyszukaniem i istniejących instancji nie dotyka.
      bucket_(&store.bucket(filename_)),
      store_(store) {
  // Kubełek z rekordami innego rozmiaru nie ma czego przekazać tej instancji: read() kopiuje rekord
  // w całości, więc szerszy wyszedłby poza bufor wołającego. Nowy plan i tak dostaje świeży kubełek
  // (poprzedni ginie z ostatnią instancją) - to jest obrona, nie droga główna.
  if (bucket_->recordSize != recordSize_) {
    bucket_->data.clear();
    bucket_->nulls.clear();
    bucket_->writeCount = 0;
    bucket_->recordSize = recordSize_;
  }
  ++bucket_->users;
}

/// Kubełek znika razem z ostatnią instancją, która go używa. Do 2026-09-27 kubełki żyły do
/// końca procesu: po `xqry --reset` strumień o tej samej nazwie dziedziczył rekordy i writeCount
/// poprzedniego planu (przesunięty pierścień, zapis poza bufor przy węższym rekordzie, FatalError
/// przy odczycie poza rozmiarem kubełka), a nazwy nieobecne w nowym planie zostawały w pamięci.
memoryFile::~memoryFile() {
  if (--bucket_->users == 0) store_.erase(filename_);
}

auto memoryFile::name() -> std::string & { return filename_; }

ssize_t memoryFile::write(const uint8_t *ptrData, const std::vector<bool> &nullBitset, const size_t position) {
  // Akcesor na deskryptorze zerowej szerokosci: rozmiar poza kontraktem (EINVAL), nie asercja.
  if (recordSize_ == 0) return EINVAL;
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
    if (!retained(bucket, location, retentionSize_)) {
      SPDLOG_ERROR("Write failed: record {} outside retention window of {} records, written {}", location, retentionSize_,
                   bucket.writeCount);
      return ERANGE;
    }
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
  if (recordSize_ == 0) return EINVAL;  // uzasadnienie w write()
  assertBucketMatches(bucket_, store_, filename_);
  auto &bucket        = *bucket_;
  const auto location = position / recordSize_;
  if (!retained(bucket, location, retentionSize_)) {
    SPDLOG_ERROR("Read failed: record {} outside retention window of {} records, written {}", location, retentionSize_,
                 bucket.writeCount);
    return ERANGE;
  }
  const size_t slot = (retentionSize_ != no_retention) ? (location % retentionSize_) : location;

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

Result<size_t> memoryFile::count() {
  assertBucketMatches(bucket_, store_, filename_);
  return bucket_->writeCount;
}

size_t memoryFile::bucketCountForUnitTest() { return MemoryStore::processDefault().size(); }

}  // namespace rdb
