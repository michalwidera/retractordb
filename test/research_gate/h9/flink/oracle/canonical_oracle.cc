// Oracle kanonicznego serializera K26 — strona C++.
//
// Program NIE zawiera wlasnej implementacji metryki: buduje rdb::Descriptor z opisu
// tekstowego i wola rdb::probe::canonicalRecordBytes(), czyli DOKLADNIE te funkcje,
// ktora liczy metryke pierwotna w silniku (src/rdb/lib/probe.cc). Dzieki temu
// porownanie z Java nie jest porownaniem dwoch przepisan tej samej specyfikacji,
// tylko porownaniem implementacji Javy z kodem silnika.
//
// Zadna linia retractordb nie jest tu modyfikowana — program linkuje sie z librdb.a
// zbudowanym w drzewie build/ (patrz build_oracle.sh).
//
// Wejscie:  plik wektorow (label <TAB> pola <TAB> oczekiwane) — kolumna oczekiwana
//           jest ignorowana, oracle jest zrodlem prawdy.
// Wyjscie:  label <TAB> bajty  na stdout, po jednym wektorze na wiersz.

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <magic_enum/magic_enum.hpp>

#include "fldType.hpp"
#include "rdb/descriptor.hpp"
#include "rdb/probe.hpp"

namespace {

std::vector<std::string> split(const std::string &text, char separator) {
  std::vector<std::string> parts;
  std::istringstream stream(text);
  std::string item;
  while (std::getline(stream, item, separator)) parts.push_back(item);
  return parts;
}

rdb::Descriptor parseDescriptor(const std::string &spec) {
  rdb::Descriptor descriptor;
  if (spec == "-") return descriptor;

  for (const auto &fieldSpec : split(spec, ';')) {
    const auto parts = split(fieldSpec, ':');
    if (parts.size() != 4) {
      std::cerr << "zle pole: " << fieldSpec << '\n';
      std::exit(2);
    }
    const auto type = magic_enum::enum_cast<rdb::descFld>(parts[3]);
    if (!type) {
      std::cerr << "nieznany typ: " << parts[3] << '\n';
      std::exit(2);
    }
    descriptor.emplace_back(parts[0], std::stoi(parts[1]), std::stoi(parts[2]), *type);
  }
  return descriptor;
}

}  // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "uzycie: canonical_oracle <plik_wektorow.tsv>\n";
    return 2;
  }

  std::ifstream input(argv[1]);
  if (!input) {
    std::cerr << "nie moge otworzyc: " << argv[1] << '\n';
    return 2;
  }

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line.front() == '#') continue;

    const auto columns = split(line, '\t');
    if (columns.size() < 2) continue;

    auto descriptor = parseDescriptor(columns[1]);
    std::cout << columns[0] << '\t' << rdb::probe::canonicalRecordBytes(descriptor) << '\n';
  }
  return 0;
}
