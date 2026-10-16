// Testy sladu stosu (stackTrace.hpp)
//
// Osobna, mala binarka celowo: zaplecze addr2line uruchamia osobny proces dla kazdej ramki,
// a kazdy z nich czyta cale DWARF binarki. W test_contracts (ok. 93 MB) jedno wywolanie
// currentStackTrace() trwalo ok. 62 s, w binarce kilku MB - ulamek sekundy.

#include <gtest/gtest.h>

#include "rdb/stackTrace.hpp"

static_assert(noexcept(rdb::currentStackTrace()));

TEST(StackTrace, returns_a_nonempty_trace_of_the_current_thread) { EXPECT_FALSE(rdb::currentStackTrace().empty()); }
