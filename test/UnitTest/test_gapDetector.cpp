// Scenariusze użycia klasy GapDetector
//
// GapDetector to czysta maszyna stanów (zero I/O) wydzielona z metaData: decyduje,
// które kolejne rekordy all-null stanowią przerwę w transmisji. Te same przypadki co
// dawny scenariusz 8 w test_metaData_usage.cpp, ale bez tworzenia pliku na dysku.

#include <gtest/gtest.h>

#include "rdb/gapDetector.hpp"

TEST(GapDetectorTest, disabled_by_default) {
  rdb::GapDetector d;
  EXPECT_FALSE(d.enabled());
}

TEST(GapDetectorTest, configure_enables_it) {
  rdb::GapDetector d;
  d.configure(2);
  EXPECT_TRUE(d.enabled());
}

// ---------------------------------------------------------------------------
// Faza nullfill: pierwsze nullFillCount obserwacji all-null przechodzą do zapisu
// (absorb() zwraca false — wywołujący ma je zapisać fizycznie, R17).
// ---------------------------------------------------------------------------
TEST(GapDetectorTest, nullfill_phase_passes_through) {
  rdb::GapDetector d;
  d.configure(2);

  EXPECT_FALSE(d.absorb(true));  // 1. rekord nullfill
  EXPECT_FALSE(d.absorb(true));  // 2. rekord nullfill

  EXPECT_EQ(d.takePendingGap(), 0U);  // nic jeszcze nie pochłonięte
}

// ---------------------------------------------------------------------------
// Poza fazą nullfill: kolejne obserwacje all-null są pochłaniane, akumulując
// długość oczekującej przerwy.
// ---------------------------------------------------------------------------
TEST(GapDetectorTest, absorbs_beyond_nullfill_phase) {
  rdb::GapDetector d;
  d.configure(2);

  d.absorb(true);  // nullfill 1
  d.absorb(true);  // nullfill 2

  EXPECT_TRUE(d.absorb(true));  // pochłonięty — przerwa=1
  EXPECT_TRUE(d.absorb(true));  // pochłonięty — przerwa=2

  EXPECT_EQ(d.takePendingGap(), 2U);
  EXPECT_EQ(d.takePendingGap(), 0U);  // odebrane — reset do 0
}

// ---------------------------------------------------------------------------
// Pierwszy rekord nie-null po przerwie: nie jest pochłaniany (caller ma go
// zapisać), a oczekująca przerwa czeka na takePendingGap() — GapDetector
// sam nie decyduje, co z nią zrobić (brak I/O w tej klasie).
// ---------------------------------------------------------------------------
TEST(GapDetectorTest, non_null_after_gap_leaves_pending_gap_for_caller) {
  rdb::GapDetector d;
  d.configure(2);

  d.absorb(true);
  d.absorb(true);
  d.absorb(true);  // pochłonięty — przerwa=1

  EXPECT_FALSE(d.absorb(false));  // rekord nie-null: nie pochłonięty
  EXPECT_EQ(d.takePendingGap(), 1U);

  // Licznik fazy nullfill wyzerowany — kolejne 2 rekordy all-null znów przechodzą.
  EXPECT_FALSE(d.absorb(true));
  EXPECT_FALSE(d.absorb(true));
}

// ---------------------------------------------------------------------------
// Bez configure() maszyna jest nieaktywna — absorb() zawsze zwraca false,
// niezależnie od wzorca (to sam wywołujący sprawdza enabled() przed absorb()).
// ---------------------------------------------------------------------------
TEST(GapDetectorTest, disabled_detector_still_reports_no_pending_gap) {
  rdb::GapDetector d;
  EXPECT_EQ(d.takePendingGap(), 0U);
}
