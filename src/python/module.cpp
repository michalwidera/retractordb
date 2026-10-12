#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/vector.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>
#include <boost/rational.hpp>
#include <magic_enum/magic_enum.hpp>

#include "fatalError.hpp"
#include "fldType.hpp"
#include "rdb/accessorFactory.hpp"
#include "rdb/descriptor.hpp"
#include "rdb/descriptorIO.hpp"
#include "rdb/embed/engine.hpp"
#include "rdb/embed/storageGuard.hpp"
#include "rdb/payload.hpp"
#include "rdb/storage.hpp"

/// @file
/// @brief Modul rozszerzenia CPythona: odczyt magazynow i osadzony Engine.
///
/// Modul udostepnia odczyt istniejacych magazynow i rdb::embed::Engine z osobnym Memory Store.
/// Engine uzywa TimeLine i dataModel, bez zegara, watku IPC i terminala. Etap 1a
/// wiazal tylko magazyn, zanim oddzielono plan i wykonanie od IPC uslugi.
/// Stan modulu i historia etapu 1a: docs/jupyter-integration.md, sekcje 1-2.
///
/// BLEDY (decyzja (a) z #463). FatalError (K1) i EnvironmentError (K4) koncza
/// proces std::exit - poza zakresem rdb::ThrowOnFatalScope. Kazde wejscie do
/// silnika idzie tu przez engineCall(), ktory zaklada ten zakres, wiec K1/K4
/// przychodza jako rdb::EngineFatal i wychodza do Pythona jako InternalError /
/// IOError zamiast zabijac interpreter. Odmowy, ktore silnik zglasza wartoscia
/// (rdb::Refusal z tryLoadDescriptorFile i attachDescriptorStatus), wychodza
/// jako CorruptDescriptor albo IOError.
///
/// TABELA KLAS K1-K7 (#476, docs/jupyter-integration.md sekcja 3). K1 skaza caly
/// modul (wariant (a)): pierwszy InternalError zapamietuje komunikat, a kazde
/// nastepne wejscie do silnika zglasza InternalError do restartu interpretera
/// (UntaintedGuard). K4 to OSError z errno (Refusal::err, EIO bez kodu). K6
/// (formy przestarzale) wychodzi z compile() jako DeprecationWarning.
///
/// Znana luka trybu rzucaj: FatalError w funkcji noexcept albo w destruktorze
/// wolanym przy zwijaniu stosu nadal konczy proces (fatalError.hpp, #463).
///
/// Straze ponizej sprawdzaja przed wywolaniem te warunki, dla ktorych Python
/// ma wlasny typ wyjatku: brak pliku deskryptora, brak katalogu magazynu, puste
/// argumenty, nieznane pole, indeks poza zakresem, odczyt ze zrodla deklarowanego.
///
/// ETAP J1-J3 (#446, C3; docs/core-phase-3.md). Plan i jego wykonanie weszly do
/// wiazania przez fasade rdb::embed::Engine: compile, step/run, rekordy i gesta
/// projekcja pol (_block), z ktorej Python sklada okna. Engine sam zaklada
/// ThrowOnFatalScope, wiec K1/K4 przychodza z niego jako rdb::EngineFatal, a
/// odmowy K2/K3 jako rdb::embed::Refused - oba tlumaczy translateEngineErrors().
/// Hierarchia (decyzja w #491):
///   RetractorDBError
///   +- ConfigError (+ValueError)       K3: Refused etapu Request, nieznany storage_type
///   |  +- PlanError                    K2: odmowa planu
///   |     +- RQLSyntaxError            Refused etapu Parse
///   |     +- CompileError              Refused etapu Check
///   |     +- CorruptDescriptor         odmowa deskryptora (#473)
///   +- IOError (+OSError)              K4: Refusal / EngineFatal klasy Environment
///   +- InternalError                   K1: EngineFatal klasy Internal
/// Trzy decyzje uzytecznosci: GIL jest zwalniany na czas compile/step/run/_block,
/// run() sprawdza sygnaly co kSignalCheckInterval, a logi silnika ida do modulu
/// `logging` (PythonLoggingSink), nie na stdout.

namespace nb = nanobind;

namespace {

/// Hierarchia bledow - typy C++ rzucane przez to wiazanie. Odmowy i bledy
/// krytyczne silnika (Refused, EngineFatal) maja wlasne typy i tlumacza.
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

/// K3: zly argument wywolania, ktory silnik zglosilby jako K1 (ConfigError).
struct RdbConfigError : RdbError {
  using RdbError::RdbError;
};

/// K4: srodowisko odmowilo (plik, dysk). Python dostaje OSError(errno, komunikat[, sciezka]);
/// EngineFatal kodu nie niesie, wiec wtedy EIO - ten sam kod, ktorym konczy sie usluga.
struct RdbIOError : RdbError {
  explicit RdbIOError(const std::string &message, const int err = 0, std::string path = {})
      : RdbError(message),
        err(err != 0 ? err : EIO),
        path(std::move(path)) {}
  int err;
  std::string path;
};

/// Odczyt i blokada maja wspolnego wlasciciela. Kopia shared_ptr na czas read() chroni przed
/// close() z innego watku, gdy readRecord zwalnia GIL; blokada schodzi dopiero po magazynie.
struct BoundStorage {
  struct State {
    std::unique_ptr<rdb::embed::StorageGuard> guard;
    std::unique_ptr<rdb::storage> store;
  };
  std::shared_ptr<State> state;

  [[nodiscard]] std::shared_ptr<State> require() const {
    if (!state) throw RdbStorageError("storage is closed");
    return state;
  }
  void close() noexcept { state.reset(); }
};

/// Skazenie modulu po K1 (wariant (a) z #463): stan silnika po zlamanym niezmienniku jest nieznany,
/// a statyki procesu (rejestry, Memory Store domyslny) dzielone przez wszystkie instancje. Pisane
/// tylko z GIL-em (tlumacz bledow), czytane takze bez niego (engineCall pod gil_scoped_release),
/// stad flaga atomowa wystawiana PO komunikacie.
struct InternalFailure {
  std::atomic<bool> raised{false};
  std::string first;
};

InternalFailure &internalFailure() {
  static InternalFailure failure;
  return failure;
}

void markInternalFailure(const std::string &message) {
  InternalFailure &failure = internalFailure();
  if (failure.raised.load(std::memory_order_acquire)) return;
  failure.first = message;
  failure.raised.store(true, std::memory_order_release);
}

void requireUntainted() {
  const InternalFailure &failure = internalFailure();
  if (!failure.raised.load(std::memory_order_acquire)) return;
  throw RdbInternalError("retractordb is unusable after an internal error (class K1) - restart the interpreter. First error: " +
                         failure.first);
}

/// nb::call_guard dla metod Engine: konstruktor rzuca, zanim wywolanie wejdzie do silnika. Stoi
/// PRZED gil_scoped_release, wiec sprawdza z GIL-em.
struct UntaintedGuard {
  UntaintedGuard() { requireUntainted(); }
};

/// Wywolanie silnika w trybie rzucaj: K1/K4 jako wyjatek Pythona zamiast std::exit.
/// Zakres konczy sie przed blokiem catch, wiec rzut ponizej nie jest juz w trybie rzucaj.
template <typename F>
decltype(auto) engineCall(F &&call) {
  requireUntainted();
  try {
    const rdb::ThrowOnFatalScope scope;
    return std::forward<F>(call)();
  } catch (const rdb::EngineFatal &fault) {
    if (fault.cls == rdb::ErrorClass::Environment) throw RdbIOError(fault.message);
    throw RdbInternalError(fault.message);
  }
}

/// Typy Pythona o DWOCH bazach (ConfigError: ValueError, IOError: OSError) i ich
/// pochodne. nb::exception przyjmuje jedna baze, wiec te powstaja przez
/// PyErr_NewException; odwolania trzyma modul (atrybuty), a te wskazniki tylko
/// je pozyczaja, wiec nie ma tu niczego do zwolnienia po Py_Finalize.
struct PlanErrorTypes {
  PyObject *config    = nullptr;
  PyObject *rqlSyntax = nullptr;
  PyObject *compile   = nullptr;
  PyObject *io        = nullptr;
  PyObject *internal  = nullptr;
};

PlanErrorTypes &planErrorTypes() {
  static PlanErrorTypes types;
  return types;
}

/// Nowy typ wyjatku `name` o bazach `bases`, dopiety do modulu.
PyObject *addExceptionType(nb::module_ &m, const char *name, const nb::tuple &bases) {
  const std::string qualified = std::string("retractordb._core.") + name;
  PyObject *type              = PyErr_NewException(qualified.c_str(), bases.ptr(), nullptr);
  if (type == nullptr) throw nb::python_error();
  m.attr(name) = nb::steal(type);
  return type;
}

/// IOError(errno, komunikat[, sciezka]) - krotka argumentow wypelnia OSError.errno, .strerror i .filename.
void setIOError(const std::string &message, const int err, const std::string &path) {
  const nb::object args = path.empty() ? nb::make_tuple(err, message) : nb::make_tuple(err, message, path);
  PyErr_SetObject(planErrorTypes().io, args.ptr());
}

/// K1 -> InternalError, a modul skazony od tej chwili (tlumacz biegnie z GIL-em).
void setInternalError(const std::string &message) {
  markInternalFailure(message);
  PyErr_SetString(planErrorTypes().internal, message.c_str());
}

/// Tlumacz bledow, ktorych nie da sie zarejestrowac przez nb::exception: typy o dwoch
/// bazach (RdbIOError -> IOError, RdbConfigError -> ConfigError), K1 ze skazeniem modulu
/// oraz bledy fasady, ktore nie sa typami tego modulu. Wyjatek, ktorego tu nie ma,
/// wychodzi z tlumacza i nanobind probuje nastepnego.
void translateEngineErrors(const std::exception_ptr &error, void * /*payload*/) {
  const PlanErrorTypes &types = planErrorTypes();
  try {
    std::rethrow_exception(error);
  } catch (const rdb::embed::Refused &refused) {
    if (refused.refusal().cls == rdb::ErrorClass::Environment) {
      setIOError(refused.what(), refused.refusal().err != 0 ? refused.refusal().err : EIO, {});
      return;
    }
    PyObject *type = types.config;
    if (refused.stage() == rdb::embed::Refused::Stage::Parse) {
      type = types.rqlSyntax;
    } else if (refused.stage() == rdb::embed::Refused::Stage::Check) {
      type = types.compile;
    }
    PyErr_SetString(type, refused.what());
  } catch (const rdb::EngineFatal &fault) {
    if (fault.cls == rdb::ErrorClass::Environment) {
      setIOError(fault.message, EIO, {});
    } else {
      setInternalError(fault.message);
    }
  } catch (const RdbIOError &failure) {
    setIOError(failure.what(), failure.err, failure.path);
  } catch (const RdbInternalError &failure) {
    setInternalError(failure.what());
  } catch (const RdbConfigError &failure) {
    PyErr_SetString(types.config, failure.what());
  }
}

/// Odmowa zgloszona wartoscia -> wyjatek. Pusty komunikat to sukces. `path` - plik, o ktory
/// pytal wolajacy, jesli go zna (OSError.filename).
void raiseIfRefused(const rdb::Refusal &refusal, const std::string &path = {}) {
  if (refusal.message.empty()) return;
  if (refusal.cls == rdb::ErrorClass::Environment) throw RdbIOError(refusal.message, refusal.err, path);
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
/// Tryb jednoliniowy nalezy do strumienia (slot xalloc, faza 2 - #446, C2), a
/// strumien jest tu lokalny: nie ma czego zapamietywac ani przywracac, a repr()
/// nie zmienia formatu zadnego innego wypisu.
template <typename T>
std::string streamToString(const T &value) {
  return engineCall([&value] {
    std::ostringstream out;
    out << rdb::singleLineFormat << value;
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

/// Most logow: spdlog -> logging.getLogger("retractordb").
///
/// Silnik pisze przez domyslny logger spdloga i NICZEGO w nim nie konfiguruje (bramka
/// embedding_boundary) - konfiguracja nalezy do hosta, a hostem jest ten modul. Domyslny sink
/// spdloga pisze na stdout, czyli w notatniku do wyjscia kazdej komorki; zamiast tego kazdy rekord
/// trafia do modulu `logging`, gdzie uzytkownik filtruje go tak, jak reszte swoich logow.
///
/// Wolno tu dotknac domyslnego loggera: spdlog jest header-only i wlinkowany w modul z ukryta
/// widocznoscia symboli (NB_STATIC), wiec jego rejestr jest PRYWATNY dla _core.
///
/// GIL: sink bywa wolany z wnetrza step()/run(), gdzie GIL jest zwolniony, wiec bierze go sam. Po
/// Py_Finalize interpretera nie ma, a destruktory statykow C++ potrafia jeszcze logowac - stad
/// straz na poczatku.
class PythonLoggingSink final : public spdlog::sinks::base_sink<std::mutex> {
 protected:
  void sink_it_(const spdlog::details::log_msg &msg) override {
    if (Py_IsInitialized() == 0) return;
    const int level = pythonLevel(msg.level);
    const std::string text(msg.payload.data(), msg.payload.size());
    const nb::gil_scoped_acquire acquire;
    try {
      nb::module_::import_("logging").attr("getLogger")("retractordb").attr("log")(level, "%s", text);
    } catch (nb::python_error &error) {
      // Log, ktorego nie da sie dostarczyc, nie ma prawa przerwac slotu silnika.
      error.discard_as_unraisable("retractordb logging bridge");
    }
  }

  void flush_() override {}

 private:
  /// Poziomy modulu logging: DEBUG=10, INFO=20, WARNING=30, ERROR=40, CRITICAL=50.
  static int pythonLevel(const spdlog::level::level_enum level) {
    switch (level) {
      case spdlog::level::trace:
      case spdlog::level::debug:
        return 10;
      case spdlog::level::info:
        return 20;
      case spdlog::level::warn:
        return 30;
      case spdlog::level::err:
        return 40;
      case spdlog::level::critical:
        return 50;
      default:
        return 0;
    }
  }
};

/// Co ile run() zaglada do sygnalow Pythona. Rzadziej niz co slot, bo wziecie GIL-a to koszt w
/// petli, ktora ma byc szybka; czesciej niz reakcja czlowieka na przycisk stop.
constexpr std::chrono::milliseconds kSignalCheckInterval{50};

/// Petla run(): wiele slotow z ZWOLNIONYM GIL-em i z okresowym PyErr_CheckSignals.
///
/// Uzytkownik notatnika nacisnie "stop", a KeyboardInterrupt dociera do Pythona wylacznie przez
/// PyErr_CheckSignals wolane z GIL-em. Bez tego run() bez budzetu slotow byloby martwym jadrem az
/// do SIGKILL.
std::uint64_t runEngine(rdb::embed::Engine &self, const std::optional<std::uint64_t> slots) {
  std::uint64_t done = 0;
  const nb::gil_scoped_release release;
  auto lastCheck = std::chrono::steady_clock::now();
  while (!slots.has_value() || done < *slots) {
    if (!self.step().has_value()) break;
    ++done;
    if (const auto now = std::chrono::steady_clock::now(); now - lastCheck >= kSignalCheckInterval) {
      lastCheck = now;
      const nb::gil_scoped_acquire acquire;
      if (PyErr_CheckSignals() != 0) throw nb::python_error();
    }
  }
  return done;
}

/// Splaszczony blok double z silnika -> tablica numpy (rows x cols) w zadanym typie, z WLASNA
/// pamiecia (kapsula zwalnia ja, gdy tablica ginie). Kopia jest zamierzona: "kopiuj, nie
/// przypinaj" z docs/jupyter-integration.md sekcja 4.
template <typename T>
nb::object makeBlock(const std::vector<double> &values, const std::size_t rows, const std::size_t cols) {
  auto data = std::make_unique<T[]>(values.size());
  for (std::size_t i = 0; i < values.size(); ++i) {
    if constexpr (std::is_integral_v<T>) {
      // Null nie ma reprezentacji w typie calkowitym; zero byloby cicha wartoscia.
      if (std::isnan(values[i])) throw nb::value_error("stream holds null values; use dtype float32 or float64");
      // Gorna granica jest wylaczna: max int64 zaokragla sie w double do 2^63.
      const double limit     = std::ldexp(1.0, std::numeric_limits<T>::digits);
      const double truncated = std::trunc(values[i]);
      if (!std::isfinite(truncated) || truncated < -limit || truncated >= limit)
        throw nb::value_error("value is outside integer dtype range; use dtype float32 or float64");
    }
    data[i] = static_cast<T>(values[i]);
  }
  T *raw = data.release();
  const nb::capsule owner(raw, [](void *p) noexcept { delete[] static_cast<T *>(p); });
  return nb::cast(nb::ndarray<nb::numpy, T, nb::ndim<2>>(raw, {rows, cols}, owner));
}

nb::object engineBlock(rdb::embed::Engine &self, const std::string &stream, const std::vector<int> &flatFields,
                       const std::size_t first, const std::size_t count, const std::string &dtype) {
  std::vector<double> values;
  {
    const nb::gil_scoped_release release;
    values = self.project(stream, flatFields, first, count);
  }
  const std::size_t cols = flatFields.size();
  if (dtype == "float32") return makeBlock<float>(values, count, cols);
  if (dtype == "float64") return makeBlock<double>(values, count, cols);
  if (dtype == "int32") return makeBlock<std::int32_t>(values, count, cols);
  if (dtype == "int64") return makeBlock<std::int64_t>(values, count, cols);
  throw nb::value_error("dtype must be one of: float32, float64, int32, int64");
}

/// Nazwa strumienia przychodzi wprost od uzytkownika notatnika, wiec literowka jest przypadkiem
/// NORMALNYM i dostaje KeyError - ta sama regula co field_index() wyzej.
void requireStream(const rdb::embed::Engine &self, const std::string &stream) {
  const auto names = self.streams();
  if (std::ranges::find(names, stream) == names.end()) throw nb::key_error(stream.c_str());
}

}  // namespace

NB_MODULE(_core, m) {
  m.doc() = "RetractorDB embedded: storage layer and the embedded engine. See docs/jupyter-integration.md.";

  // Kolejnosc rejestracji ma znaczenie: nanobind probuje tlumaczy w kolejnosci
  // ODWROTNEJ do rejestracji, a typy pochodne lapia sie rowniez na catch po
  // klasie bazowej. Baza pierwsza => pochodne sprawdzane wczesniej.
  const nb::object baseError = nb::exception<RdbError>(m, "RetractorDBError");
  nb::exception<RdbNoSuchStream>(m, "NoSuchStream", baseError);
  nb::exception<RdbStorageError>(m, "StorageError", baseError);
  const nb::object internalError = nb::exception<RdbInternalError>(m, "InternalError", baseError);

  // Typy o dwoch bazach i hierarchia bledow planu (decyzja w #491): `except ValueError` lapie K2
  // i K3, `except OSError` - K4, `except RetractorDBError` - wszystko z modulu.
  PlanErrorTypes &types = planErrorTypes();
  types.internal        = internalError.ptr();
  types.config          = addExceptionType(m, "ConfigError", nb::make_tuple(baseError, nb::handle(PyExc_ValueError)));
  PyObject *planError   = addExceptionType(m, "PlanError", nb::make_tuple(nb::handle(types.config)));
  types.rqlSyntax       = addExceptionType(m, "RQLSyntaxError", nb::make_tuple(nb::handle(planError)));
  types.compile         = addExceptionType(m, "CompileError", nb::make_tuple(nb::handle(planError)));
  types.io              = addExceptionType(m, "IOError", nb::make_tuple(baseError, nb::handle(PyExc_OSError)));
  // Odmowa deskryptora to K2 (#473), wiec pod PlanError; jedna baza, wiec wystarcza nb::exception.
  nb::exception<RdbCorruptDescriptor>(m, "CorruptDescriptor", nb::handle(planError));
  // Po nb::exception: tlumacze ida w kolejnosci odwrotnej, wiec ten - z RdbInternalError, ktory
  // skaza modul - jest probowany pierwszy.
  nb::register_exception_translator(translateEngineErrors);

  // Most logow instalowany raz, przy imporcie: od tej chwili kazdy zapis silnika przez spdlog trafia
  // do logging.getLogger("retractordb"). Poziom loggera spdloga zdejmujemy do trace, bo filtrowanie
  // ma nalezec do `logging`; to, co kompilacja wyciela przez SPDLOG_ACTIVE_LEVEL, i tak tu nie dociera.
  spdlog::default_logger()->sinks() = {std::make_shared<PythonLoggingSink>()};
  spdlog::default_logger()->set_level(spdlog::level::trace);

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
        raiseIfRefused(engineCall([&] { return rdb::tryLoadDescriptorFile(path, descriptor); }), path);
        return descriptor;
      },
      nb::arg("path"), "Read a .desc file. A file that holds an invalid descriptor raises CorruptDescriptor.");

  nb::class_<BoundStorage>(m, "Storage")
      .def(nb::new_([](const std::string &qry_id, const std::string &file_name, const std::string &storage_param,
                       const std::string &storage_type, const std::string &lock_dir, const std::string &config) {
             requireUntainted();
             // Straze przed konstruktorem StoragePaths i attachDescriptor - kazdy z
             // tych warunkow ma w Pythonie wlasny typ wyjatku.
             if (qry_id.empty()) throw nb::value_error("qry_id must not be empty");
             if (file_name.empty()) throw nb::value_error("file_name must not be empty");
             if (!storage_param.empty() && !std::filesystem::is_directory(storage_param)) {
               throw RdbNoSuchStream("storage_param is not a directory: " + storage_param);
             }
             // Lista typow nalezy do fabryki akcesorow; silnik zglasza nieznany typ jako K1, a K1
             // skazilby caly modul za literowke w argumencie.
             if (!rdb::isDeclaredType(storage_type) && !rdb::isWritableType(storage_type)) {
               throw RdbConfigError("unsupported storage_type '" + storage_type + "'");
             }

             const rdb::embed::OfflinePolicy offline = rdb::embed::offlinePolicy(lock_dir, config);
             rdb::embed::checkOffline(offline);
             // Bez lock_dir i paths.lock_dir katalog blokad serwisu jest zgadywany (TMPDIR tego procesu,
             // a serwis spod systemd blokuje zwykle w /tmp). Cisza dawalaby falszywe poczucie ochrony:
             // UserWarning na ekran (raz na miejsce wywolania), a ten sam tekst do logu hosta na INFO.
             if (offline.lockDir.empty()) {
               constexpr const char *guessed =
                   "Storage: no service lock directory given, so the running-service check looked into TMPDIR as a "
                   "guess; pass config= with the service's TOML file or lock_dir= matching its paths.lock_dir";
               nb::module_::import_("logging").attr("getLogger")("retractordb").attr("info")("%s", guessed);
               if (PyErr_WarnEx(PyExc_UserWarning, guessed, 1) != 0) throw nb::python_error();
             }
             auto created   = std::make_unique<BoundStorage>();
             created->state = std::make_shared<BoundStorage::State>();
             auto &state    = *created->state;
             state.guard    = std::make_unique<rdb::embed::StorageGuard>(storage_param, false);
             if (state.guard->ownsDirectory()) {
               const std::filesystem::path directory(state.guard->directory());
               state.guard->checkPath((directory / (qry_id + ".desc")).string());
               state.guard->checkPath((directory / (qry_id + ".meta")).string());
               for (const auto &suffix : {"", ".shadow", ".meta", ".meta.shadow"})
                 state.guard->checkPath((directory / file_name).string() + suffix);
             }
             state.store =
                 engineCall([&] { return std::make_unique<rdb::storage>(qry_id, file_name, storage_param, storage_type); });
             if (!state.store->descriptorFileExist()) throw RdbNoSuchStream("no descriptor file for stream: " + qry_id);
             raiseIfRefused(engineCall([&] { return state.store->attachDescriptorStatus(nullptr); }));
             return created;
           }),
           nb::arg("qry_id"), nb::arg("file_name"), nb::arg("storage_param") = "", nb::arg("storage_type") = "DEFAULT",
           nb::arg("lock_dir") = "", nb::arg("config") = "",
           "Open storage offline. lock_dir must match the service's paths.lock_dir; config names a TOML file read "
           "only when given (paths.lock_dir, embedded.lenient_lock_dirs), and lock_dir wins over it. Without either "
           "the check looks into TMPDIR. Listed lock directories, or the system temporary directory without the key, "
           "are checked leniently, like xtrdb. The startup check does not exclude a later daemon start. Owned "
           "embedded directories stay locked until close().")
      .def_prop_ro("descriptor", [](const BoundStorage &self) { return self.require()->store->descriptor; })
      .def_prop_ro("record_count",
                   [](const BoundStorage &self) {
                     const auto state = self.require();
                     return engineCall([&] { return state->store->getRecordsCount(); });
                   })
      .def_prop_ro("is_declared", [](const BoundStorage &self) { return self.require()->store->isDeclared(); })
      .def_prop_ro("closed", [](const BoundStorage &self) { return !self.state; })
      .def("close", &BoundStorage::close, "Close the storage and release its directory lock. Idempotent.")
      .def(
          "read",
          [](const BoundStorage &self, Py_ssize_t index) {
            const auto state = self.require();
            return readRecord(*state->store, index);
          },
          nb::arg("index"), "Read one record. A negative index counts from the end, which is what revRead() does in C++.")
      .def("__getitem__",
           [](const BoundStorage &self, Py_ssize_t index) {
             const auto state = self.require();
             return readRecord(*state->store, index);
           })
      .def("__len__",
           [](const BoundStorage &self) {
             const auto state = self.require();
             return engineCall([&] { return state->store->getRecordsCount(); });
           })
      .def(
          "__enter__",
          [](BoundStorage &self) {
            static_cast<void>(self.require());
            return &self;
          },
          nb::rv_policy::reference_internal)
      // .none() na kazdym argumencie NIE jest ozdoba: w nanobindzie argument
      // typu obiektowego domyslnie NIE przyjmuje None, a `with` przy wyjsciu bez
      // wyjatku podaje dokladnie (None, None, None). Bez tego kazdy blok `with`
      // konczy sie TypeError - przy czym cialo bloku juz sie wykonalo.
      .def(
          "__exit__",
          [](BoundStorage &self, nb::handle, nb::handle, nb::handle) {
            self.close();
            return false;
          },
          nb::arg("exc_type").none(), nb::arg("exc").none(), nb::arg("traceback").none())
      .def("__repr__", [](const BoundStorage &self) {
        return self.state ? "<Storage records=" + std::to_string(self.state->store->getRecordsCount()) + ">"
                          : std::string("<Storage (closed)>");
      });

  // Silnik osadzony. Metody z oknami i iteracja po rekordach sa dopisane po stronie Pythona
  // (retractordb/engine.py), ktory dziedziczy po tej klasie: numpy i logika okien nie maja powodu
  // byc w C++.
  nb::class_<rdb::embed::Engine>(m, "Engine",
                                 "One embedded engine: a compiled plan, driven one time slot at a time. See "
                                 "docs/jupyter-integration.md.")
      .def(nb::init<std::string, std::string, std::string>(), nb::arg("storage_dir") = "", nb::arg("lock_dir") = "",
           nb::arg("config") = "", nb::call_guard<UntaintedGuard>(),
           "An empty storage_dir creates a private temporary directory, which skips the service lock check; an "
           "explicit directory must be empty or owned. STORAGE in the plan overrides it under the same checks. "
           "lock_dir must match the service's paths.lock_dir; config names a TOML file read only when given "
           "(paths.lock_dir, embedded.lenient_lock_dirs), and lock_dir wins over it.")
      // string_view wskazuje bufor UTF-8 obiektu str, ktory zyje przez cale wywolanie, wiec
      // zwolnienie GIL-a na czas parsowania i kompilacji jest bezpieczne. Ostrzezenia K6 ida z
      // powrotem z GIL-em; stacklevel 1 przypisuje je ramce, ktora wolala compile().
      .def(
          "compile",
          [](rdb::embed::Engine &self, const std::string_view rql, const bool untilEof) {
            {
              const nb::gil_scoped_release release;
              self.compile(rql, untilEof);
            }
            for (const std::string &warning : self.warnings())
              if (PyErr_WarnEx(PyExc_DeprecationWarning, warning.c_str(), 1) != 0) throw nb::python_error();
          },
          nb::arg("rql"), nb::arg("until_eof") = true, nb::call_guard<UntaintedGuard>(),
          "Parse, compile and build the plan. Raises RQLSyntaxError or CompileError (both PlanError). With "
          "until_eof=True (the default) declared sources are read once, without wrapping, and step() reports end "
          "of input. Deprecated RQL forms are reported as DeprecationWarning.")
      .def("step", &rdb::embed::Engine::step, nb::call_guard<UntaintedGuard, nb::gil_scoped_release>(),
           "Advance one time slot. Returns the slot index, or None at end of input.")
      .def("run", &runEngine, nb::arg("slots") = nb::none(), nb::call_guard<UntaintedGuard>(),
           "Advance up to `slots` slots (all, until end of input, when None). Returns the number of slots "
           "processed. Releases the GIL and honours KeyboardInterrupt.")
      .def_prop_ro("has_plan", &rdb::embed::Engine::hasPlan)
      .def_prop_ro("storage_dir", &rdb::embed::Engine::storageDir)
      .def_prop_ro("slots_done", &rdb::embed::Engine::slotsDone)
      .def_prop_ro("end_of_input", &rdb::embed::Engine::endOfInput)
      .def_prop_ro("failed", &rdb::embed::Engine::failed,
                   "True after step() raised: the plan stopped mid-slot and must be compiled again.")
      .def_prop_ro(
          "time",
          [](const rdb::embed::Engine &self) {
            const auto now = self.time();
            return fractionType()(now.numerator(), now.denominator());
          },
          "Plan time of the last slot, in seconds, as a fractions.Fraction.")
      .def("streams", &rdb::embed::Engine::streams, nb::call_guard<UntaintedGuard>(),
           "Stream ids of the plan, in execution order.")
      .def(
          "schema",
          [](const rdb::embed::Engine &self, const std::string &stream) {
            requireStream(self, stream);
            return self.schema(stream);
          },
          nb::call_guard<UntaintedGuard>(), nb::arg("stream"), nb::rv_policy::copy)
      .def(
          "is_declared",
          [](const rdb::embed::Engine &self, const std::string &stream) {
            requireStream(self, stream);
            return self.isDeclared(stream);
          },
          nb::call_guard<UntaintedGuard>(), nb::arg("stream"))
      .def(
          "record_count",
          [](const rdb::embed::Engine &self, const std::string &stream) {
            requireStream(self, stream);
            return self.recordCount(stream);
          },
          nb::call_guard<UntaintedGuard>(), nb::arg("stream"))
      .def(
          "retained_from",
          [](const rdb::embed::Engine &self, const std::string &stream) {
            requireStream(self, stream);
            return self.retainedFrom(stream);
          },
          nb::call_guard<UntaintedGuard>(), nb::arg("stream"),
          "Index of the oldest record still readable: 0 for a stream on disk, higher for a declared source or a "
          "VOLATILE stream, which keep only a tail.")
      .def(
          "record",
          [](rdb::embed::Engine &self, const std::string &stream, Py_ssize_t index) {
            requireStream(self, stream);
            const auto count = static_cast<Py_ssize_t>(self.recordCount(stream));
            if (index < 0) index += count;
            if (index < 0 || index >= count) throw nb::index_error("record index out of range");
            if (std::cmp_less(index, self.retainedFrom(stream))) {
              throw nb::index_error("record is no longer retained; see retained_from()");
            }
            rdb::payload data;
            {
              const nb::gil_scoped_release release;
              data = self.record(stream, static_cast<std::size_t>(index));
            }
            return Record{std::move(data), static_cast<std::size_t>(index)};
          },
          nb::call_guard<UntaintedGuard>(), nb::arg("stream"), nb::arg("index"),
          "One record of a stream, oldest first; a negative index counts from the end. A copy.")
      .def(
          "_block",
          [](rdb::embed::Engine &self, const std::string &stream, const std::vector<int> &flatFields, const std::size_t first,
             const std::size_t count, const std::string &dtype) {
            requireStream(self, stream);
            return engineBlock(self, stream, flatFields, first, count, dtype);
          },
          nb::call_guard<UntaintedGuard>(), nb::arg("stream"), nb::arg("flat_fields"), nb::arg("first"), nb::arg("count"),
          nb::arg("dtype"), "Dense (count x len(flat_fields)) numpy block of records [first, first+count); null -> NaN.")
      .def("close", &rdb::embed::Engine::close, "Drop the plan and close its storages. Idempotent.")
      .def(
          "__enter__", [](rdb::embed::Engine &self) { return &self; }, nb::rv_policy::reference_internal)
      .def(
          "__exit__",
          [](rdb::embed::Engine &self, nb::handle, nb::handle, nb::handle) {
            self.close();
            return false;
          },
          nb::arg("exc_type").none(), nb::arg("exc").none(), nb::arg("traceback").none())
      .def("__repr__", [](const rdb::embed::Engine &self) {
        return self.hasPlan() ? "<Engine streams=" + std::to_string(self.streams().size()) +
                                    " slots_done=" + std::to_string(self.slotsDone()) + ">"
                              : std::string("<Engine (no plan)>");
      });
}
