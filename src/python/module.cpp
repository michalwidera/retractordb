#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/unique_ptr.h>
#include <boost/rational.hpp>
#include <magic_enum/magic_enum.hpp>

#include "fatalError.hpp"
#include "fldType.hpp"
#include "rdb/descriptor.hpp"
#include "rdb/descriptorIO.hpp"
#include "rdb/payload.hpp"
#include "rdb/storage.hpp"

/// @file
/// @brief Etap 1a osadzania: modul rozszerzenia CPythona nad warstwa magazynu.
///
/// Wiazanie jest CELOWO tylko do odczytu i konczy sie na warstwie magazynu.
/// Wszystko wyzej (qTree, dataModel, executorsm) jest uslugowe i trzyma stan
/// globalny - wiec zwiazane dzis daloby API dzialajace raz na interpreter.
/// Uzasadnienie i granice etapu: docs/jupyter-integration.md.
///
/// BLEDY (decyzja (a) z #463). FatalError (K1) i EnvironmentError (K4) koncza
/// proces std::exit - poza zakresem rdb::ThrowOnFatalScope. Kazde wejscie do
/// silnika idzie tu przez engineCall(), ktory zaklada ten zakres, wiec K1/K4
/// przychodza jako rdb::EngineFatal i wychodza do Pythona jako InternalError /
/// IOError zamiast zabijac interpreter. Odmowy, ktore silnik zglasza wartoscia
/// (rdb::Refusal z tryLoadDescriptorFile i attachDescriptorStatus), wychodza
/// jako CorruptDescriptor albo IOError. Pelna tabela klas K1-K7 w API i stan
/// procesu po K1 w trybie osadzonym - #476.
///
/// Znana luka trybu rzucaj: FatalError w funkcji noexcept albo w destruktorze
/// wolanym przy zwijaniu stosu nadal konczy proces (fatalError.hpp, #463).
///
/// Straze ponizej sprawdzaja przed wywolaniem te warunki, dla ktorych Python
/// ma wlasny typ wyjatku: brak pliku deskryptora, brak katalogu magazynu, puste
/// argumenty, nieznane pole, indeks poza zakresem, odczyt ze zrodla deklarowanego.

namespace nb = nanobind;

namespace {

/// Hierarchia bledow. Rejestrowane sa tylko typy, ktore ten etap umie zglosic;
/// bledy planu (RQLSyntaxError, CompileError) wejda razem z silnikiem (J1).
struct RdbError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct RdbNoSuchStream : RdbError {
  using RdbError::RdbError;
};

struct RdbStorageError : RdbError {
  using RdbError::RdbError;
};

struct RdbCorruptDescriptor : RdbError {
  using RdbError::RdbError;
};

/// K1: zlamany niezmiennik silnika - blad w NASZYM kodzie, do zgloszenia, nie do obsluzenia.
struct RdbInternalError : RdbError {
  using RdbError::RdbError;
};

/// K4: srodowisko odmowilo (plik, dysk).
struct RdbIOError : RdbError {
  using RdbError::RdbError;
};

/// Wywolanie silnika w trybie rzucaj: K1/K4 jako wyjatek Pythona zamiast std::exit.
/// Zakres konczy sie przed blokiem catch, wiec rzut ponizej nie jest juz w trybie rzucaj.
template <typename F>
decltype(auto) engineCall(F &&call) {
  try {
    const rdb::ThrowOnFatalScope scope;
    return std::forward<F>(call)();
  } catch (const rdb::EngineFatal &fault) {
    if (fault.cls == rdb::ErrorClass::Environment) throw RdbIOError(fault.message);
    throw RdbInternalError(fault.message);
  }
}

/// Odmowa zgloszona wartoscia -> wyjatek. Pusty komunikat to sukces.
void raiseIfRefused(const rdb::Refusal &refusal) {
  if (refusal.message.empty()) return;
  if (refusal.cls == rdb::ErrorClass::Environment) throw RdbIOError(refusal.message);
  throw RdbCorruptDescriptor(refusal.message);
}

/// Nazwa pola przychodzi wprost od uzytkownika notatnika, wiec literowka jest
/// przypadkiem NORMALNYM i dostaje KeyError; silnik zglasza ja jako K1.
void requireField(const rdb::Descriptor &self, const std::string_view name) {
  if (!self.hasField(name)) throw nb::key_error(std::string(name).c_str());
}

/// Typ fractions.Fraction, pobierany raz. boost::rational<int> nie ma w Pythonie
/// odpowiednika wbudowanego, a krotka (licznik, mianownik) zlewalaby sie z
/// INTPAIR - oba sa para intow, a znacza co innego.
nb::object fractionType() {
  static const nb::object cached = nb::module_::import_("fractions").attr("Fraction");
  return cached;
}

/// descFldVT -> obiekt Pythona. std::monostate oraz brak wartosci (pole null)
/// daja None; rozroznienie miedzy nimi nie niesie informacji dla czytajacego.
nb::object toPython(const std::optional<rdb::descFldVT> &value) {
  if (!value.has_value()) return nb::none();

  return std::visit(Overload{
                        [](const std::monostate &) { return nb::object(nb::none()); },
                        [](const std::uint8_t arg) { return nb::cast(static_cast<int>(arg)); },
                        [](const int arg) { return nb::cast(arg); },
                        [](const unsigned int arg) { return nb::cast(arg); },
                        [](const float arg) { return nb::cast(arg); },
                        [](const double arg) { return nb::cast(arg); },
                        [](const boost::rational<int> &arg) { return fractionType()(arg.numerator(), arg.denominator()); },
                        [](const std::pair<int, int> &arg) { return nb::cast(arg); },
                        [](const std::pair<std::string, int> &arg) { return nb::cast(arg); },
                        [](const std::string &arg) { return nb::cast(arg); },
                    },
                    value.value());
}

/// Rekord odczytany z magazynu.
///
/// Trzyma WLASNA kopie payloadu, a nie wskaznik do bufora magazynu. Leniwosc,
/// o ktora chodzi, dotyczy nie tworzenia obiektu Pythona na pole przy odczycie -
/// nie wspoldzielenia pamieci. Widok na bufor magazynu uniewaznialby sie przy
/// nastepnym read() i dawalby ciche czytanie cudzych danych; kopia rekordu to
/// kilkadziesiat bajtow.
struct Record {
  rdb::payload data;
  std::size_t index;
};

/// Sformatuj obiekt przez jego operator<< - deskryptor i payload maja wlasne.
///
/// Tryb jednoliniowy trzeba PRZYWROCIC. Manipulator singleLineFormat ustawia
/// statyczna flage calego procesu i nigdy jej nie cofa, wiec pojedyncze repr()
/// w notatniku zmienialoby format kazdego pozniejszego wypisu deskryptora.
/// Flage per strumien wnosi faza 2 (#446, C2).
template <typename T>
std::string streamToString(const T &value) {
  return engineCall([&value] {
    const bool previous = rdb::Descriptor::isSingleLineOutput();
    rdb::Descriptor::setSingleLineOutput(true);
    std::ostringstream out;
    out << value;
    rdb::Descriptor::setSingleLineOutput(previous);
    return out.str();
  });
}

/// Odczyt rekordu pod straza.
///
/// storage::read() dla indeksu poza zakresem zwraca NoSuchRecord z payloadem
/// all-null - zakres sprawdzamy wiec tutaj, zeby Python dostal IndexError, a nie
/// rekord z samych None. Odczyt ze zrodla deklarowanego (BINFILE, TEXTFILE,
/// DEVICE) to w silniku K1, wiec zatrzymujemy go przed wywolaniem.
Record readRecord(rdb::storage &self, Py_ssize_t index) {
  if (self.isDeclared()) {
    throw RdbStorageError("cannot read directly from a declared (BINFILE/TEXTFILE/DEVICE) storage");
  }

  const auto count = static_cast<Py_ssize_t>(self.getRecordsCount());
  if (index < 0) index += count;  // indeks ujemny od konca - zastepuje revRead()
  if (index < 0 || index >= count) throw nb::index_error("record index out of range");

  auto *const target = self.getPayload();
  if (target == nullptr) throw RdbStorageError("storage has no payload attached");

  {
    // GIL zwalniany na czas samego wejscia-wyjscia. Bez tego jeden odczyt
    // zamraza cale jadro razem z jego interfejsem, a dolozenie tej straznicy
    // pozniej oznacza przeglad wszystkich wywolan, ktore do tego czasu powstana.
    const nb::gil_scoped_release release;
    // Status bez znaczenia: indeks sprawdzony wyzej wzgledem liczby rekordow.
    engineCall([&] { static_cast<void>(self.read(static_cast<std::size_t>(index))); });
  }

  // Kopia payloadu powstaje juz z GIL-em: to samo przepisanie pamieci, a Record
  // ma byc niezalezny od nastepnego read().
  return Record{*target, static_cast<std::size_t>(index)};
}

}  // namespace

NB_MODULE(_core, m) {
  m.doc() = "RetractorDB storage layer, embedded (stage 1a). See docs/jupyter-integration.md.";

  // Kolejnosc rejestracji ma znaczenie: nanobind probuje tlumaczy w kolejnosci
  // ODWROTNEJ do rejestracji, a typy pochodne lapia sie rowniez na catch po
  // klasie bazowej. Baza pierwsza => pochodne sprawdzane wczesniej.
  const nb::object baseError = nb::exception<RdbError>(m, "RetractorDBError");
  nb::exception<RdbNoSuchStream>(m, "NoSuchStream", baseError);
  nb::exception<RdbStorageError>(m, "StorageError", baseError);
  nb::exception<RdbCorruptDescriptor>(m, "CorruptDescriptor", baseError);
  nb::exception<RdbInternalError>(m, "InternalError", baseError);
  nb::exception<RdbIOError>(m, "IOError", baseError);

  // Nazwy pozycji wyliczenia biora sie z magic_enum, zeby nie rozjechac sie z
  // fldType.hpp przy dodaniu typu. reserve() jest WYMAGANE, nie kosmetyczne:
  // nb::enum_ dostaje wskazniki const char*, a realokacja wektora uniewaznilaby
  // te juz przekazane.
  static std::vector<std::string> enumNames;
  enumNames.reserve(magic_enum::enum_count<rdb::descFld>());
  auto fieldType = nb::enum_<rdb::descFld>(m, "FieldType");
  for (const auto &[value, name] : magic_enum::enum_entries<rdb::descFld>()) {
    enumNames.emplace_back(name);
    fieldType.value(enumNames.back().c_str(), value);
  }

  nb::class_<rdb::rField>(m, "Field")
      .def_ro("name", &rdb::rField::rname)
      .def_ro("length", &rdb::rField::rlen)
      .def_ro("array_count", &rdb::rField::rarray)
      .def_ro("type", &rdb::rField::rtype)
      .def("__repr__", [](const rdb::rField &self) {
        return "<Field " + self.rname + " " + std::string(rdb::GetStringdescFld(self.rtype)) + "[" +
               std::to_string(self.rarray) + "] len=" + std::to_string(self.rlen) + ">";
      });

  nb::class_<rdb::Descriptor>(m, "Descriptor")
      .def_prop_ro("size_bytes", [](const rdb::Descriptor &self) { return engineCall([&] { return self.getSizeInBytes(); }); })
      .def_prop_ro("flat_element_count",
                   [](const rdb::Descriptor &self) { return engineCall([&] { return self.flatElementCount(); }); })
      .def("has_field", &rdb::Descriptor::hasField, nb::arg("name"))
      .def(
          "field_index",
          [](const rdb::Descriptor &self, const std::string &name) {
            requireField(self, name);
            return engineCall([&] { return self.fieldIndex(name); });
          },
          nb::arg("name"))
      .def(
          "byte_offset",
          [](const rdb::Descriptor &self, const std::string &name) {
            requireField(self, name);
            return engineCall([&] { return self.fieldByteOffset(name); });
          },
          nb::arg("name"))
      .def(
          "field_type_name",
          [](const rdb::Descriptor &self, const std::string &name) {
            requireField(self, name);
            return engineCall([&] { return std::string(self.fieldTypeName(name)); });
          },
          nb::arg("name"))
      .def("storage_policy", [](rdb::Descriptor &self) { return engineCall([&] { return self.storagePolicy(); }); })
      .def("__len__", [](const rdb::Descriptor &self) { return self.size(); })
      .def("__getitem__",
           [](const rdb::Descriptor &self, Py_ssize_t position) {
             // Indeks ujemny liczony od konca - protokol sekwencji Pythona;
             // bez tego descriptor[-1] czytalby spod wskaznika za koncem.
             const auto count = static_cast<Py_ssize_t>(self.size());
             if (position < 0) position += count;
             if (position < 0 || position >= count) throw nb::index_error("field index out of range");
             return self[static_cast<std::size_t>(position)];
           })
      .def("__repr__", [](const rdb::Descriptor &self) { return streamToString(self); });

  nb::class_<Record>(m, "Record")
      .def_ro("index", &Record::index)
      .def_prop_ro("descriptor", [](const Record &self) { return self.data.descriptor; })
      .def("__len__", [](const Record &self) { return self.data.descriptor.flatElementCount(); })
      .def("__getitem__",
           [](const Record &self, Py_ssize_t position) {
             const auto count = static_cast<Py_ssize_t>(self.data.descriptor.flatElementCount());
             if (position < 0) position += count;
             if (position < 0 || position >= count) throw nb::index_error("value index out of range");
             return toPython(engineCall([&] { return self.data.getItemVT(static_cast<int>(position)); }));
           })
      .def("__repr__", [](const Record &self) { return streamToString(self.data); });

  m.def(
      "load_descriptor",
      [](const std::string &path) {
        // Brak pliku to najczestsza pomylka w notatniku - wlasny typ, nie odmowa I/O.
        if (!std::filesystem::exists(path)) throw RdbNoSuchStream("no descriptor file: " + path);
        rdb::Descriptor descriptor;
        raiseIfRefused(engineCall([&] { return rdb::tryLoadDescriptorFile(path, descriptor); }));
        return descriptor;
      },
      nb::arg("path"), "Read a .desc file. A file that holds an invalid descriptor raises CorruptDescriptor.");

  nb::class_<rdb::storage>(m, "Storage")
      .def(nb::new_([](const std::string &qry_id, const std::string &file_name, const std::string &storage_param,
                       const std::string &storage_type) {
             // Straze przed konstruktorem StoragePaths i attachDescriptor - kazdy z
             // tych warunkow ma w Pythonie wlasny typ wyjatku.
             if (qry_id.empty()) throw nb::value_error("qry_id must not be empty");
             if (file_name.empty()) throw nb::value_error("file_name must not be empty");
             if (!storage_param.empty() && !std::filesystem::is_directory(storage_param)) {
               throw RdbNoSuchStream("storage_param is not a directory: " + storage_param);
             }

             auto created =
                 engineCall([&] { return std::make_unique<rdb::storage>(qry_id, file_name, storage_param, storage_type); });
             if (!created->descriptorFileExist()) throw RdbNoSuchStream("no descriptor file for stream: " + qry_id);

             raiseIfRefused(engineCall([&] { return created->attachDescriptorStatus(nullptr); }));
             return created;
           }),
           nb::arg("qry_id"), nb::arg("file_name"), nb::arg("storage_param") = "", nb::arg("storage_type") = "DEFAULT")
      .def_prop_ro("descriptor", [](rdb::storage &self) { return self.descriptor; })
      .def_prop_ro("record_count", &rdb::storage::getRecordsCount)
      .def_prop_ro("is_declared", &rdb::storage::isDeclared)
      .def("read", &readRecord, nb::arg("index"),
           "Read one record. A negative index counts from the end, which is what revRead() does in C++.")
      .def("__getitem__", &readRecord)
      .def("__len__", [](const rdb::storage &self) { return self.getRecordsCount(); })
      .def(
          "__enter__", [](rdb::storage &self) { return &self; }, nb::rv_policy::reference_internal)
      // .none() na kazdym argumencie NIE jest ozdoba: w nanobindzie argument
      // typu obiektowego domyslnie NIE przyjmuje None, a `with` przy wyjsciu bez
      // wyjatku podaje dokladnie (None, None, None). Bez tego kazdy blok `with`
      // konczy sie TypeError - przy czym cialo bloku juz sie wykonalo.
      .def(
          "__exit__", [](rdb::storage &, nb::handle, nb::handle, nb::handle) { return false; }, nb::arg("exc_type").none(),
          nb::arg("exc").none(), nb::arg("traceback").none())
      .def("__repr__",
           [](const rdb::storage &self) { return "<Storage records=" + std::to_string(self.getRecordsCount()) + ">"; });
}
