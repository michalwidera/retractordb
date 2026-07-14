#pragma once

#include <cstddef>

namespace rdb {

/// @brief Maszyna stanów decydująca, kiedy ciąg rekordów all-null stanowi przerwę w transmisji.
///
/// Obiekt klasy GapDetector powinien:
/// - być nieaktywny (enabled() == false), dopóki configure() nie zostanie wywołane,
/// - po skonfigurowaniu (configure(nullFillCount)) przepuszczać pierwsze nullFillCount kolejnych
///   obserwacji all-null (absorb(true) zwraca false — wywołujący ma je zapisać fizycznie, faza nullfill),
/// - dalsze obserwacje all-null pochłaniać (absorb(true) zwraca true), akumulując długość oczekującej przerwy,
/// - przy pierwszej obserwacji nie-null (absorb(false)) zerować licznik fazy nullfill, zwracając false;
///   ewentualna oczekująca przerwa NIE jest tu odrzucana — pozostaje do odebrania przez takePendingGap(),
/// - udostępniać takePendingGap() zwracające zaakumulowaną długość przerwy i zerujące ją do 0,
/// - nie wykonywać żadnego I/O — decyzję, co zrobić z przerwą (np. zapisać wpis gap), podejmuje wywołujący.
class GapDetector {
 public:
  /// @brief Enable the state machine; @p nullFillCount all-null observations pass through before absorbing.
  void configure(int nullFillCount);

  /// @brief Whether configure() was called.
  [[nodiscard]] bool enabled() const { return configured_; }

  /// @brief Feed one all-null/non-null observation into the state machine.
  /// @param isAllNull whether the observed record is all-null
  /// @return true when the record was absorbed into the pending gap (caller must NOT write it to storage)
  bool absorb(bool isAllNull);

  /// @brief Take the accumulated pending gap duration and reset it to 0.
  /// @return the gap duration to flush, or 0 if nothing is pending
  size_t takePendingGap();

  /// @brief Reset the running counters (consecutive-null count, pending gap duration).
  ///
  /// Does not change enabled()/nullFillCount_ — called by metaData::reset() when clearing
  /// index data (purge/rotation), which must not silently disable gap detection.
  void resetCounters();

 private:
  int nullFillCount_{2};            ///< number of nullfill records passed through before a gap is marked (R17)
  size_t consecutiveNullCount_{0};  ///< consecutive all-null records fed into absorb()
  size_t activeGapDuration_{0};     ///< accumulated gap duration not yet taken by takePendingGap()
  bool configured_{false};          ///< true only when configure() was explicitly called
};

}  // namespace rdb
