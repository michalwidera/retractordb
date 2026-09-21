#include "rdb/descriptorIO.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <iostream>

#include "fatalError.hpp"
#include "rdb/exceptions.hpp"

namespace rdb {

Descriptor loadDescriptorFile(const std::string &descriptorFile) {
  Descriptor descriptor;

  std::fstream myFile;
  myFile.rdbuf()->pubsetbuf(nullptr, 0);
  myFile.open(descriptorFile, std::ios::in);  // Open existing descriptor

  // failbit po operator>> znaczy teraz dokladnie "tekst sie nie sparsowal" - ekstraktor
  // gasi ten ustawiony przez koniec strumienia (descriptor.cc). Stan trzeba odczytac
  // PRZED close(), bo close() na strumieniu, ktorego nie udalo sie otworzyc, sam zapala
  // failbit i zatarlby rozroznienie.
  bool parseFailed = false;
  if (myFile.good()) {
    myFile >> descriptor;
    parseFailed = myFile.fail();
  }
  myFile.close();

  // Rzut, nie FatalError: ta funkcja jest granica biblioteki i jedynym wejsciem, przez
  // ktore wiazanie Pythona wczytuje deskryptor. std::exit nie odwija stosu, wiec zaden
  // catch po stronie osadzajacego procesu go nie widzi - konczyl sie smiercia jadra
  // notatnika na jednym uszkodzonym pliku.
  if (parseFailed) {
    SPDLOG_ERROR("Invalid descriptor in file: {}", descriptorFile);
    throw CorruptDescriptor("invalid descriptor in file: " + descriptorFile);
  }

  // Deskryptor pusty rowniez wtedy, gdy pliku po prostu nie ma - myFile.good() jest
  // wowczas falszem i nic sie nie czytalo. Komunikat zostaje wspolny, bo taki byl przed
  // faza 1, a rozdzielenie brakujacego pliku od pustego nalezy do taksonomii z fazy 5.
  if (descriptor.getSizeInBytes() == 0) {
    SPDLOG_ERROR("Empty descriptor in file.");
    throw CorruptDescriptor("empty descriptor file: " + descriptorFile);
  }
  return descriptor;
}

void saveDescriptorFile(const std::string &descriptorFile, const Descriptor &descriptor) {
  std::fstream descFile;
  descFile.rdbuf()->pubsetbuf(nullptr, 0);
  descFile.open(descriptorFile, std::ios::out);
  if ((descFile.rdstate() & std::ofstream::failbit) != 0) {
    FatalError("storage: failed to open descriptor file for writing: {}", descriptorFile);
  }
  descFile << descriptor;
  if ((descFile.rdstate() & std::ofstream::failbit) != 0) {
    FatalError("storage: failed to write descriptor file: {}", descriptorFile);
  }
  descFile.close();
}

void verifyDescriptorMatch(const Descriptor &provided, const Descriptor &existing, const std::string &descriptorFile) {
  if (provided == existing) return;

  // ConfigError, nie blad wejscia-wyjscia: plik jest w porzadku, tylko opisuje inny
  // ksztalt rekordu niz ten, o ktory prosi plan. Zwykle znaczy to zmieniona deklaracje
  // strumienia nad istniejacymi danymi - czyli cos, co autor planu ma poprawic.
  //
  // Oba deskryptory ida na stderr, a nie do tekstu wyjatku: sa wielowierszowe, a komunikat
  // wyjatku bywa przekazywany dalej jednym wierszem (patrz kMaxSyntaxErrorMessage).
  SPDLOG_ERROR("Descriptors do not match in {}.", descriptorFile);
  std::cerr << "Error in data descriptor file: " << descriptorFile << '\n';
  std::cerr << "Provided Descriptor:\n" << provided << "\nExisting Descriptor:\n" << existing << '\n';
  throw ConfigError("storage: descriptor schema mismatch in " + descriptorFile + " - remove data files and restart");
}

}  // namespace rdb
