#pragma once

#include <format>
#include <string>

/// Funkcja humanDuration powinna:
/// - [humanduration.human-duration.1~1] zapisywać czas w sekundach jako `D d H h M min S s`, pomijając wiodące człony równe zeru i zostawiając zawsze sekundy, z ułamkiem uciętym, a wartość niedodatnią jako `0 s`.
///
/// @note Wspolna dla `xqry --horizon` i `xretractor -c --horizon` (#640), zeby oba wydruki horyzontu
///       podawaly czas tak samo.
[[nodiscard]] inline std::string humanDuration(const double seconds) {
  const long long rest = seconds > 0 ? static_cast<long long>(seconds) : 0LL;
  const long long d    = rest / 86400;
  const long long h    = rest / 3600 % 24;
  const long long min  = rest / 60 % 60;
  std::string text;
  if (d > 0) text += std::format("{} d ", d);
  if (d > 0 || h > 0) text += std::format("{} h ", h);
  if (d > 0 || h > 0 || min > 0) text += std::format("{} min ", min);
  return text + std::format("{} s", rest % 60);
}
