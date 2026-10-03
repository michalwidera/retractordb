#include "rdb/descriptorIO.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

#include <spdlog/spdlog.h>

#include <fmt/format.h>

extern std::string parserDESCString(rdb::Descriptor &desc, std::string_view inlet);

namespace rdb {

std::string tryLoadDescriptorFile(const std::string &descriptorFile, Descriptor &descriptor) {
  std::ifstream file(descriptorFile);
  if (!file) return "cannot open descriptor file: " + descriptorFile;

  std::ostringstream content;
  content << file.rdbuf();
  if (file.bad()) return "cannot read descriptor file: " + descriptorFile;

  Descriptor parsed;
  if (const std::string result = parserDESCString(parsed, content.str()); result != "OK")
    return "descriptor parse failed in '" + descriptorFile + "': " + result;
  if (parsed.getSizeInBytes() == 0) return "storage: empty descriptor file: " + descriptorFile;
  descriptor = std::move(parsed);
  return {};
}

Result<Descriptor> loadDescriptorFile(const std::string &descriptorFile) {
  // Blad jako wartosc, nie koniec procesu: ta funkcja jest granica biblioteki i jedynym
  // wejsciem, przez ktore wiazanie Pythona wczytuje deskryptor. Powod odmowy jest ten sam,
  // ktory serwer dostaje z tryLoadDescriptorFile() - brak pliku, blad odczytu, skladni albo
  // wartosci, pusty deskryptor - i jedna kategoria dla wszystkich, jak przed rozdzieleniem.
  Descriptor descriptor;
  if (std::string error = tryLoadDescriptorFile(descriptorFile, descriptor); !error.empty()) {
    SPDLOG_ERROR("Invalid descriptor: {}", error);
    return fail(Errc::CorruptDescriptor, std::move(error));
  }
  return descriptor;
}

Result<> saveDescriptorFile(const std::string &descriptorFile, const Descriptor &descriptor) {
  std::fstream descFile;
  descFile.rdbuf()->pubsetbuf(nullptr, 0);
  descFile.open(descriptorFile, std::ios::out);
  if ((descFile.rdstate() & std::ofstream::failbit) != 0) {
    return fail(Errc::IO, fmt::format("storage: failed to open descriptor file for writing: {}", descriptorFile));
  }
  descFile << descriptor;
  if ((descFile.rdstate() & std::ofstream::failbit) != 0) {
    return fail(Errc::IO, fmt::format("storage: failed to write descriptor file: {}", descriptorFile));
  }
  descFile.close();
  return {};
}

Result<> verifyDescriptorMatch(const Descriptor &provided, const Descriptor &existing, const std::string &descriptorFile) {
  if (provided == existing) return {};

  // Errc::Config, nie Errc::IO: plik jest w porzadku, tylko opisuje inny
  // ksztalt rekordu niz ten, o ktory prosi plan. Zwykle znaczy to zmieniona deklaracje
  // strumienia nad istniejacymi danymi - czyli cos, co autor planu ma poprawic.
  //
  // Oba deskryptory ida na stderr, a nie do tekstu bledu: sa wielowierszowe, a komunikat
  // bledu bywa przekazywany dalej jednym wierszem (patrz kMaxSyntaxErrorMessage).
  SPDLOG_ERROR("Descriptors do not match in {}.", descriptorFile);
  std::cerr << "Error in data descriptor file: " << descriptorFile << '\n';
  std::cerr << "Provided Descriptor:\n" << provided << "\nExisting Descriptor:\n" << existing << '\n';
  return fail(Errc::Config, "storage: descriptor schema mismatch in " + descriptorFile + " - remove data files and restart");
}

}  // namespace rdb
