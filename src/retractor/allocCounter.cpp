// =============================================================================
// allocCounter.cpp — globalny licznik alokacji sterty (diagnostyka inwestygacji
//                    speed_improvement). Analogiczny do RDB_COPY_COUNTER
//                    (src/retractor/lib/query.cpp), ale mierzy alokacje new/delete.
//
// Cały plik znika bez śladu bez -DRDB_ALLOC_COUNTER=ON (cały kod pod #ifdef);
// wtedy powstaje pusty obiekt, zero kosztu w produkcji.
//
// DLACZEGO W TARGECIE WYKONYWALNYM, NIE W BIBLIOTECE:
//   Zamiana globalnego operator new/delete w bibliotece STATYCZNEJ może nie
//   zostać dolinkowana — domyślny operator new z libstdc++ już rozwiązuje
//   symbol, więc linker nie ma powodu wciągać obiektu z archiwum. Kompilując ten
//   plik wprost do binarki xretractor (jawnie w SOURCES_RETRACTOR) gwarantujemy,
//   że zamiana wchodzi do obrazu wykonywalnego.
//
// JAK MIERZYĆ (metoda RÓŻNICY — deterministyczna, odporna na szum WSL2):
//   Liczniki zliczają KAŻDĄ alokację procesu (start, parsowanie ANTLR, pętla),
//   więc pojedynczy total miesza narzut startowy z kosztem pętli. Uruchom DWA
//   przebiegi o różnej liczbie interwałów N1 < N2 i policz:
//       alokacje/interwał = (alloc(N2) − alloc(N1)) / (N2 − N1)
//   Stały narzut startowy się skraca; zostaje koszt jednego interwału.
//   (Ten sam wzorzec co przy RDB_COPY_COUNTER — patrz NEXT-STEPS.md.)
// =============================================================================
#ifdef RDB_ALLOC_COUNTER

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <new>

namespace ralloc {
// Constant-initialized (constexpr atomic{0}) — dostępne zanim ruszy dynamiczna
// inicjalizacja globali, więc tracked_alloc jest bezpieczne od pierwszej
// alokacji procesu.
std::atomic<long> allocCount{0};
std::atomic<long> allocBytes{0};
std::atomic<long> freeCount{0};

namespace {
// Rejestr kubełków lokalizacyjnych (RDB_ALLOC_SCOPE). Strona instrumentowana
// rejestruje kubełki przy static init; Reporter wypisuje je przy wyjściu. Stała
// pojemność — brak alokacji w registerBucket (a i tak byłaby startowa, kasuje się
// w metodzie różnicy). const char* to literały nazw faz — nie alokują.
constexpr int kMaxBuckets = 32;
struct BucketEntry {
  const char *name;
  std::atomic<long> *counter;
};
BucketEntry g_buckets[kMaxBuckets];
int g_bucketCount = 0;

// Raport na stderr przy zakończeniu procesu (destruktor obiektu statycznego),
// tak jak Reporter w query.cpp. fprintf do (niebuforowanego) stderr nie alokuje.
struct Reporter {
  ~Reporter() {
    std::fprintf(stderr, "[RDB_ALLOC_COUNTER] heap allocs total: %ld  bytes total: %ld  frees total: %ld\n", allocCount.load(),
                 allocBytes.load(), freeCount.load());
    for (int i = 0; i < g_bucketCount; ++i)
      std::fprintf(stderr, "[RDB_ALLOC_COUNTER] bucket %s: %ld\n", g_buckets[i].name, g_buckets[i].counter->load());
  }
} reporter_;
}  // namespace

void registerBucket(const char *name, std::atomic<long> *counter) {
  if (g_bucketCount < kMaxBuckets) g_buckets[g_bucketCount++] = {name, counter};
}

inline void *tracked_alloc(std::size_t size) {
  allocCount.fetch_add(1, std::memory_order_relaxed);
  allocBytes.fetch_add(static_cast<long>(size), std::memory_order_relaxed);
  void *p = std::malloc(size ? size : 1);  // new(0) musi zwrócić unikalny wskaźnik
  if (!p) throw std::bad_alloc();
  return p;
}

inline void tracked_free(void *p) noexcept {
  if (p) freeCount.fetch_add(1, std::memory_order_relaxed);
  std::free(p);
}
}  // namespace ralloc

// Zamieniamy TYLKO rodzinę niewyrównaną (new/new[] + delete/delete[], w tym
// warianty sized-deallocation). Alokacje nadwyrównane (align_val_t) trafiają do
// domyślnych operatorów libstdc++ i pozostają NIEzliczone — kompilator paruje je
// z domyślnym aligned-delete, więc nie ma mieszania rodzin (bezpieczne). Gorąca
// ścieżka (std::any, vector<token>, payload, string, rational) nie używa typów
// nadwyrównanych, więc pomiar jest kompletny dla mierzonego kodu.
void *operator new(std::size_t size) { return ralloc::tracked_alloc(size); }
void *operator new[](std::size_t size) { return ralloc::tracked_alloc(size); }
void operator delete(void *p) noexcept { ralloc::tracked_free(p); }
void operator delete[](void *p) noexcept { ralloc::tracked_free(p); }
void operator delete(void *p, std::size_t) noexcept { ralloc::tracked_free(p); }
void operator delete[](void *p, std::size_t) noexcept { ralloc::tracked_free(p); }

#endif  // RDB_ALLOC_COUNTER
