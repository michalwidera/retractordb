// Proba toolchainu obrazu kol (docker/wheel/Dockerfile).
//
// Kazda konstrukcja nizej wystepuje w bibliotekach silnika, ktore wchodza do
// retractordb._core: std::println do FILE* (src/rdb/lib/probe.cc) i do
// std::ostream (src/retractor/lib/executorsm.cpp), std::format, std::from_chars
// dla double, std::filesystem, std::ranges::fold_left i literal uz. Z GCC 14
// wymagaja one symboli libstdc++ az do GLIBCXX_3.4.32 (zmierzone na Ubuntu 24.04,
// GCC 14.2), a polityka manylinux_2_28 dopuszcza najwyzej GLIBCXX_3.4.24.
// gcc-toolset ma dolinkowac brakujace symbole statycznie z libstdc++_nonshared.a;
// ta proba sprawdza, ze tak sie dzieje, zanim obraz zacznie budowac zaleznosci.
//
// Budowana dwa razy: jako program (link i uruchomienie na glibc 2.28) oraz jako
// biblioteka wspoldzielona z -DRDB_PROBE_LIBRARY, ktora ocenia auditwheel.

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <format>
#include <iostream>
#include <print>
#include <string>
#include <system_error>
#include <vector>

extern "C" int rdb_toolchain_probe() {
  const std::vector<int> values{1, 2, 3};
  const std::size_t sum =
      std::ranges::fold_left(values, 0uz, [](std::size_t acc, int value) { return acc + std::size_t(value); });

  const std::string text  = "2.25";
  double parsed           = 0;
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), parsed);

  const std::filesystem::path path = std::filesystem::path("/tmp/./rdb/../probe").lexically_normal();

  const std::string line = std::format("sum={} parsed={:.2f} path={}", sum, parsed, path.string());
  std::println(stdout, "{}", line);
  std::println(std::cerr, "{}", line);

  const bool ok = sum == 6 && error == std::errc{} && end == text.data() + text.size() && parsed == 2.25 &&
                  path == "/tmp/probe" && std::filesystem::exists("/");
  return ok ? 0 : 1;
}

#ifndef RDB_PROBE_LIBRARY
int main() { return rdb_toolchain_probe(); }
#endif
