#pragma once

// Limity zasobow procesu xretractor ustawiane przy starcie. Silnik osadzony (rdbembed, modul
// Pythona) ich nie zmienia - limity naleza do procesu-gospodarza.

#include <sys/resource.h>

#include "rdb/sizeLimits.hpp"

namespace proclimits {

/// Miekki RLIMIT_NOFILE, do ktorego xretractor podnosi limit przy starcie. Model danych trzyma ok.
/// 2 deskryptory na strumien magazynowany (plik danych i .shadow), regula DUMP do kMaxDumpRetention
/// otwartych zadan, a reszta przypada na log, IPC, magistrale i zapytania ad hoc. 1024 to domyslny
/// miekki limit Linuksa: na macOS (domyslnie 256) plan o kMaxPlanStreams strumieni nie miescil sie
/// w limicie (job macOS 1257, 2026-11-05), a po podniesieniu obie platformy pracuja z ta sama wartoscia.
inline constexpr rlim_t kOpenFilesTarget = 1024;
static_assert(2 * rdb::limits::kMaxPlanStreams + rdb::limits::kMaxDumpRetention < kOpenFilesTarget,
              "kOpenFilesTarget must cover the widest plan and the largest DUMP RETENTION");

struct OpenFilesLimit {
  rlim_t before = 0;  // miekki limit przed wywolaniem
  rlim_t after  = 0;  // miekki limit po wywolaniu
  rlim_t hard   = 0;  // twardy limit
  int err       = 0;  // errno z getrlimit/setrlimit, 0 przy powodzeniu
};

/// Funkcja raiseOpenFilesLimit powinna:
/// - [proclimits.raise-open-files-limit.1~1] podnosić miękki RLIMIT_NOFILE do mniejszej z wartości: target i limit twardy, i zwracać miękki limit przed i po zmianie oraz limit twardy.
/// - [proclimits.raise-open-files-limit.2~1] nie zmieniać miękkiego limitu, który jest już co najmniej równy mniejszej z wartości: target i limit twardy.
/// - [proclimits.raise-open-files-limit.3~1] błąd getrlimit albo setrlimit zwracać w polu err (errno) bez zmiany limitu i bez kończenia procesu.
///   Nietestowalne: getrlimit/setrlimit z RLIMIT_NOFILE i miekkim limitem nie wyzszym niz twardy nie zawodza; galaz bledu nie ma haka.
OpenFilesLimit raiseOpenFilesLimit(rlim_t target = kOpenFilesTarget);

}  // namespace proclimits
