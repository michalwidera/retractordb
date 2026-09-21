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

#include "fldType.hpp"
#include "rdb/descriptor.hpp"
#include "rdb/descriptorIO.hpp"
#include "rdb/payload.hpp"
#include "rdb/storage.hpp"

/// @file
/// @brief Etap 1a osadzania: modul rozszerzenia CPythona nad warstwa magazynu.
///
/// Wiazanie jest CELOWO tylko do odczytu i konczy sie na warstwie magazynu.
/// Wszystko wyzej (qTree, dataModel, executorsm) jest uslugowe - konczy proces
/// przy bledzie i trzyma stan globalny - wiec zwiazane dzis daloby API dzialajace
/// raz na interpreter. Uzasadnienie i granice etapu: docs/jupyter-integration.md.
///
/// OGRANICZENIE, ktorego to wiazanie nie usuwa: FatalError konczy sie
/// std::exit(EXIT_FAILURE) (fatalError.hpp:51), a w samej warstwie magazynu jest
/// 79 takich miejsc. std::exit nie odwija stosu, wiec zaden catch tutaj go nie
/// przechwyci - zabija interpreter razem z sesja notatnika. Zamiana tych miejsc
/// na wyjatki to faza 1 wspolnego refaktoru.
///
/// Co WOLNO zrobic juz teraz i co robimy nizej: sprawdzic w wiazaniu te warunki,
/// ktore inaczej trafilyby prosto w FatalError - brak pliku deskryptora, brak
/// katalogu magazynu, indeks poza zakresem, odczyt ze zrodla deklarowanego.
/// Kazda taka straz zamienia smierc procesu na zwyklego Pythonowego wyjatka i
/// jest jedyna ochrona, jaka warstwa wiazania moze dac przed faza 1.

namespace nb = nanobind;

namespace {

/// Hierarchia bledow. Trzy typy, nie piec: pozostale z docelowej taksonomii
/// (blad skladni RQL, blad kompilacji) nie maja dzis czego zglaszac - wejda
/// razem z faza 1, ktora da im zrodlo.
struct RdbError : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct RdbNoSuchStream : RdbError {
  using RdbError::RdbError;
};

struct RdbStorageError : RdbError {
  using RdbError::RdbError;
};

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
/// nie wspoldzielenia pamieci. Widok na bufor magazynu unieważnialby sie przy
/// nastepnym read() i dawalby ciche czytanie cudzych danych; kopia rekordu to
/// kilkadziesiat bajtow. Ten sam wybor co w projekcie DLPack dla faz J2.
struct Record {
  rdb::payload data;
  std::size_t index;
};

/// Sformatuj obiekt przez jego operator<< - deskryptor i payload maja wlasne.
///
/// Tryb jednoliniowy trzeba PRZYWROCIC. Manipulator singleLineFormat ustawia
/// statyczna flage calego procesu (descriptor.cc:282-285) i nigdy jej nie cofa,
/// wiec pojedyncze repr() w notatniku zmienialoby format kazdego pozniejszego
/// wypisu deskryptora. Dokladnie ta klasa stanu globalnego jest przedmiotem
/// fazy 2 wspolnego refaktoru - nie dokladamy do niej kolejnego przypadku.
template <typename T>
std::string streamToString(const T &value) {
  const bool previous = rdb::Descriptor::isSingleLineOutput();
  rdb::Descriptor::setSingleLineOutput(true);
  std::ostringstream out;
  out << value;
  rdb::Descriptor::setSingleLineOutput(previous);
  return out.str();
}

/// Odczyt rekordu pod straza.
///
/// storage::read() NIE konczy sie bledem dla indeksu poza zakresem - loguje i
/// oddaje wyzerowany rekord (storage.cc:181). Cicha bledna wartosc jest gorsza
/// niz wyjatek, wiec zakres sprawdzamy tutaj. Odczyt ze zrodla deklarowanego
/// (DEVICE, TEXTSOURCE) trafia z kolei prosto w FatalError, wiec tez zatrzymujemy
/// go przed wywolaniem.
Record readRecord(rdb::storage &self, Py_ssize_t index) {
  if (self.isDeclared()) {
    throw RdbStorageError("cannot read directly from a declared (DEVICE/TEXTSOURCE) storage");
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
    self.read(static_cast<std::size_t>(index));
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

  // Nazwy pozycji wyliczenia biora sie z magic_enum, zeby nie rozjechac sie z
  // fldType.hpp przy dodaniu typu. reserve() jest WYMAGANE, nie kosmetyczne:
  // nb::enum_ dostaje wskazniki const char*, a realokacja wektora unieważniłaby
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
      .def_prop_ro("size_bytes", [](const rdb::Descriptor &self) { return self.getSizeInBytes(); })
      .def_prop_ro("flat_element_count", [](const rdb::Descriptor &self) { return self.flatElementCount(); })
      .def("has_field", &rdb::Descriptor::hasField, nb::arg("name"))
      .def("field_index", &rdb::Descriptor::fieldIndex, nb::arg("name"))
      .def("byte_offset", &rdb::Descriptor::fieldByteOffset, nb::arg("name"))
      .def("field_type_name", &rdb::Descriptor::fieldTypeName, nb::arg("name"))
      .def("storage_policy", &rdb::Descriptor::storagePolicy)
      .def("__len__", [](const rdb::Descriptor &self) { return self.size(); })
      .def("__getitem__",
           [](const rdb::Descriptor &self, Py_ssize_t position) {
             // Indeks ujemny liczony od konca - protokol sekwencji Pythona;
             // bez tego descriptor[-1] czytalby spod wskaznika za koncem.
             const auto count = static_cast<Py_ssize_t>(self.size());
             if (position < 0) position += count;
             if (position < 0 || position >= count) throw nb::index_error("field index out of range");
             return self.at(static_cast<std::size_t>(position));
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
             return toPython(self.data.getItemVT(static_cast<int>(position)));
           })
      .def("__repr__", [](const Record &self) { return streamToString(self.data); });

  m.def(
      "load_descriptor",
      [](const std::string &path) {
        // Straz przed FatalError w loadDescriptorFile: brak pliku to najczestsza
        // pomylka w notatniku, a bez tego konczy sie zabiciem jadra.
        if (!std::filesystem::exists(path)) throw RdbNoSuchStream("no descriptor file: " + path);
        return rdb::loadDescriptorFile(path);
      },
      nb::arg("path"),
      "Read a .desc file. A file that exists but holds an invalid descriptor still ends the process - see the module "
      "docstring.");

  nb::class_<rdb::storage>(m, "Storage")
      .def(nb::new_([](const std::string &qry_id, const std::string &file_name, const std::string &storage_param,
                       const std::string &storage_type) {
             // Trzy straze przed konstruktorem StoragePaths i attachDescriptor -
             // kazdy z tych warunkow trafia inaczej prosto w FatalError.
             if (qry_id.empty()) throw nb::value_error("qry_id must not be empty");
             if (file_name.empty()) throw nb::value_error("file_name must not be empty");
             if (!storage_param.empty() && !std::filesystem::is_directory(storage_param)) {
               throw RdbNoSuchStream("storage_param is not a directory: " + storage_param);
             }

             auto created = std::make_unique<rdb::storage>(qry_id, file_name, storage_param, storage_type);
             if (!created->descriptorFileExist()) throw RdbNoSuchStream("no descriptor file for stream: " + qry_id);

             created->attachDescriptor(nullptr);
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
      // konczy sie TypeError - przy czym ciala bloku juz sie wykonalo.
      .def(
          "__exit__", [](rdb::storage &, nb::handle, nb::handle, nb::handle) { return false; }, nb::arg("exc_type").none(),
          nb::arg("exc").none(), nb::arg("traceback").none())
      .def("__repr__",
           [](const rdb::storage &self) { return "<Storage records=" + std::to_string(self.getRecordsCount()) + ">"; });
}
