#include "planSource.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <format>
#include <sstream>
#include <system_error>
#include <tuple>

#include <spdlog/spdlog.h>

#include "persistentCounter.hpp"
#include "rdb/accessorFactory.hpp"
#include "rdb/descriptorIO.hpp"
#include "rdb/storageShadow.hpp"
#include "RQLParser.hpp"

namespace {

void dropArtifactFile(const std::filesystem::path &artifact_filename) {
  std::error_code ec;  // przeciazenia z error_code: blad stat() to "pliku nie widac", nie wyjatek
  if (std::filesystem::exists(artifact_filename, ec)) {
    std::filesystem::remove(artifact_filename, ec);
    if (ec) {
      SPDLOG_WARN("Failed to remove file {}: {}", artifact_filename.string(), ec.message());
    }
  }
}

/// Typ magazynu rozstrzygamy DOKLADNIE tak, jak zrobi to wykonanie: `VOLATILE` wpisuje `TYPE` do
/// deskryptora (query::descriptorStorage), a ten w rdb::storage::attachStorage wygrywa z polityka
/// z klauzuli `STORAGE`.
bool isMemoryStream(const query &q) { return ((q.policy.second != 0) ? q.policy.first : q.storage_policy) == "MEMORY"; }

/// Pelna rodzina plikow wezla na dysku: deskryptor `<id>.desc` oraz wszystko, co rosnie przy pliku
/// danych `<nazwa>` (FILE albo id - regula streamInstance): sam plik z cieniem, indeks .meta z cieniem
/// i segmenty retencji `<nazwa>_segment_N` z cieniami (nazwy jak w fagrp.cc i faccposixshd.cc).
/// Do 2026-09-27 znikaly tylko `<id>`, `<id>.desc` i `<id>.meta`: stare segmenty wracaly do groupFile
/// pod swieze metadane (FatalError "record count mismatch" przy drugim starcie strumienia z
/// `RETENTION c s`), a pliki posrednich wezlow rosly miedzy restartami.
void dropArtifactFamily(const std::filesystem::path &dir, const query &q) {
  const std::filesystem::path data = dir / (q.filename.empty() ? q.id : q.filename);
  const std::string meta           = data.string() + ".meta";
  for (const std::filesystem::path &file :
       {dir / (q.id + ".desc"), data, std::filesystem::path(data.string() + ".shadow"), std::filesystem::path(meta),
        std::filesystem::path(rdb::storageShadow::metaShadowFilePath(meta))})
    dropArtifactFile(file);

  const std::string prefix           = data.filename().string() + "_segment_";
  const std::filesystem::path parent = data.parent_path().empty() ? std::filesystem::path(".") : data.parent_path();
  // Petla z increment(ec), nie range-for: operator++ iteratora katalogu zglasza blad odczytu
  // katalogu wyjatkiem, a tu ma on tylko zakonczyc sprzatanie.
  std::error_code ec;
  for (std::filesystem::directory_iterator it(parent, ec), end; !ec && it != end; it.increment(ec)) {
    const auto &entry      = *it;
    const std::string name = entry.path().filename().string();
    std::string_view rest  = name;
    if (!rest.starts_with(prefix)) continue;
    rest.remove_prefix(prefix.size());
    if (rest.ends_with(".shadow")) rest.remove_suffix(std::string_view(".shadow").size());
    if (!rest.empty() && std::ranges::all_of(rest, [](unsigned char c) { return std::isdigit(c) != 0; }))
      dropArtifactFile(entry.path());
  }
}

/// Wartosc pierwszego pola konfiguracyjnego danego rodzaju (TYPE, REF); pusta, gdy go nie ma.
std::string configField(const rdb::Descriptor &descriptor, const rdb::descFld kind) {
  const auto it = std::ranges::find_if(descriptor, [kind](const auto &item) { return item.rtype == kind; });
  return (it == descriptor.end()) ? std::string{} : it->rname;
}

/// `.desc` deklaracji zapisany przed #346 dla zwyklego pliku binarnego: wtedy kazde zrodlo
/// nietekstowe dostawalo TYPE DEVICE, teraz taki plik ma TYPE BINFILE. Poza typem plik musi
/// sie zgadzac z planem we wszystkim - REF i slotach - bo tylko wtedy jest to ten sam
/// strumien zapisany starsza nazwa typu, a nie inne zrodlo.
bool isLegacyBinaryDescriptor(const rdb::Descriptor &planned, const rdb::Descriptor &kept) {
  return configField(kept, rdb::TYPE) == "DEVICE" && configField(planned, rdb::TYPE) == "BINFILE" &&
         configField(kept, rdb::REF) == configField(planned, rdb::REF) && planned == kept;
}

/// Konfiguracja magazynu w postaci, w jakiej operator zapisalby ja w planie.
std::string describeStore(const std::string &type, const rdb::retention_t &retention) {
  if (retention.noRetention()) return std::format("STORAGE {} without RETENTION", type);
  return std::format("STORAGE {} RETENTION {} {}", type, retention.capacity, retention.segments);
}

rdb::Refusal checkDescriptorPath(const std::filesystem::path &path, bool &exists) {
  struct stat info{};
  if (::lstat(path.c_str(), &info) == 0) {
    exists = true;
    if (!S_ISREG(info.st_mode) && !S_ISLNK(info.st_mode))
      return {.cls = rdb::ErrorClass::Plan, .message = std::format("descriptor file '{}' is not a regular file", path.string())};
    return {};
  }
  const int err = errno;
  exists        = false;
  if (err == ENOENT) return {};
  return {.cls     = rdb::ErrorClass::Environment,
          .message = std::format("cannot inspect descriptor file '{}': {}", path.string(), std::strerror(err)),
          .err     = err};
}

rdb::Refusal checkTextFields(const rdb::Descriptor &descriptor, const std::string &stream) {
  for (const auto &field : descriptor) {
    if (field.rlen == 0) continue;
    switch (field.rtype) {
      case rdb::BYTE:
      case rdb::INTEGER:
      case rdb::UINT:
      case rdb::FLOAT:
      case rdb::DOUBLE:
      case rdb::STRING:
        break;
      default:
        return {.cls     = rdb::ErrorClass::Plan,
                .message = std::format("TEXTFILE stream '{}': unsupported field type {} for field '{}'", stream,
                                       descriptor.fieldTypeName(field.rname), field.rname)};
    }
  }
  return {};
}

}  // namespace

PlanSource parsePlanText(qTree &plan, const std::string &text, std::string_view sourceFile) {
  PlanSource retVal;
  std::istringstream source(text);
  // Numer wiersza idzie do parsera, bo tylko tutaj wiadomo, w ktorym miejscu pliku stoi
  // instrukcja: parser dostaje ja wyjeta z kontekstu i sam liczylby od jedynki.
  std::vector<std::string> statementKeywords;
  // Stale LET (#440) obowiazuja w kolejnych instrukcjach tego samego planu i nigdzie dalej.
  RqlConstants constants;
  for (const auto &[stmt, firstLine] : readLogicalLines(source)) {
    auto [status, first_keyword, stream_name] = parserRQLString(plan, stmt, statementKeywords, firstLine, sourceFile, constants);
    if (status != "OK") {
      retVal.status = status;
      return retVal;
    }
    retVal.lines.emplace_back(stream_name, stmt);
  }
  // Ostrzezenie, nie blad (#440, O2): nieuzyta stala nie zmienia planu, ale zwykle znaczy literowke.
  // Zmienna VAR (#531) dostaje to samo ostrzezenie.
  std::vector<std::tuple<size_t, std::string, bool>> unused;
  for (const auto &[name, constant] : constants)
    if (!constant.used) unused.emplace_back(constant.line, name, constant.variable.has_value());
  std::ranges::sort(unused);
  for (const auto &[line, name, variable] : unused)
    retVal.warnings.push_back(
        std::format("line {}: {} ?{} is declared but never used", line, variable ? "variable" : "constant", name));
  return retVal;
}

void dropStalePlanArtifacts(const qTree &plan) {
  // :ROTATION zachowuje historie wezlow plikowych, wiec ich artefakty zostaja - ich zgodnosc z planem
  // sprawdza wczesniej checkKeptStores(). Strumien MEMORY danych na dysku nie ma, a jego .desc i .meta
  // to konfiguracja POPRZEDNIEGO przebiegu: magazyn bierze TYPE i RETMEMORY z wczytanego .desc, nie
  // z planu. Do 2026-09-27 rotacja nie kasowala niczego, wiec .desc zapisany przed 108a5e94 (bez
  // RETMEMORY) przywracal pierscien bez granicy.
  //
  // Wezly bierzemy z planu, nie z linii zapisu: tylko plan zna instancje generatora (`cell$0`..)
  // i wezly posrednie, ktore kompilator wydzielil z wyrazen - ich pliki do 2026-09-27 zostawaly.
  const bool rotation             = std::ranges::any_of(plan, [](const auto &it) { return it.id == ":ROTATION"; });
  const std::filesystem::path dir = planStorageDir(plan, {});
  for (const auto &q : plan) {
    if (q.isCompilerDirective()) continue;
    if (q.isDeclaration()) {
      // Magazyn bierze TYPE z wczytanego `.desc`, nie z planu, wiec dawny TYPE DEVICE zwyklego
      // pliku binarnego (sprzed #346) musi zniknac, zanim magazyn go przeczyta. Deklaracja nie
      // ma w katalogu magazynu zadnych danych - `.desc` zapisze sie od nowa z planu.
      // checkDescriptorFiles() przepuszcza wylacznie ten przypadek.
      // Bez error_code `exists` rzuca filesystem_error (np. EACCES), a rdzen nie obsluguje wyjatkow. Blad
      // sprawdzenia zostawia `.desc` - kontrola deskryptorow przy otwarciu magazynu zglosi go z przyczyna.
      const std::filesystem::path descFile = dir / (q.id + ".desc");
      std::error_code ec;
      rdb::Descriptor kept;
      if (std::filesystem::exists(descFile, ec) && rdb::tryLoadDescriptorFile(descFile.string(), kept).message.empty() &&
          isLegacyBinaryDescriptor(query(q).descriptorStorage(), kept)) {
        SPDLOG_INFO("Stream {}: replacing pre-#346 TYPE DEVICE in {} with TYPE BINFILE", q.id, descFile.string());
        dropArtifactFile(descFile);
      }
      continue;
    }
    if (rotation && !isMemoryStream(q)) continue;
    dropArtifactFamily(dir, q);
  }
}

rdb::Refusal checkKeptStores(qTree &plan, const std::string_view defaultStorageDir,
                             const std::vector<std::string> &streamNames) {
  if (std::ranges::none_of(plan, [](const auto &it) { return it.id == ":ROTATION"; })) return {};
  const std::filesystem::path dir = planStorageDir(plan, defaultStorageDir);
  for (auto &q : plan) {
    if (q.isDeclaration() || q.isCompilerDirective() || isMemoryStream(q)) continue;
    if (!streamNames.empty() && std::ranges::find(streamNames, q.id) == streamNames.end()) continue;
    const std::filesystem::path descFile = dir / (q.id + ".desc");
    bool exists                          = false;
    if (const auto error = checkDescriptorPath(descFile, exists); !error.message.empty()) return error;
    if (!exists) continue;

    // Ta sama regula co rdb::storage::attachStorage: TYPE z wczytanego .desc wygrywa, bez niego
    // obowiazuje STORAGE z planu.
    rdb::Descriptor kept;
    if (const auto error = rdb::tryLoadDescriptorFile(descFile.string(), kept); !error.message.empty()) return error;
    const auto keptType     = kept.storagePolicy().first.empty() ? q.storage_policy : kept.storagePolicy().first;
    const auto keptStore    = describeStore(keptType, kept.retention());
    const auto plannedStore = describeStore(q.storageType(), q.descriptorStorage().retention());
    if (keptStore == plannedStore) continue;
    return {.cls = rdb::ErrorClass::Plan,
            .message =
                std::format("Stream '{}' keeps its files under ROTATION, but {} was written with {} and the plan asks for {}: "
                            "put {} back in the plan, or remove the stream's files to start it afresh",
                            q.id, descFile.string(), keptStore, plannedStore, keptStore)};
  }
  return {};
}

rdb::Refusal checkDescriptorFiles(qTree &plan, const std::string_view defaultStorageDir,
                                  const std::vector<std::string> &streamNames) {
  const bool rotation             = std::ranges::any_of(plan, [](const auto &q) { return q.id == ":ROTATION"; });
  const std::filesystem::path dir = planStorageDir(plan, defaultStorageDir);
  for (auto &q : plan) {
    if (q.isCompilerDirective()) continue;
    // Wymiana planu kasuje wyniki bez ROTATION i MEMORY. Import ad-hoc ich nie kasuje,
    // wiec dla listy nowych nazw badamy kazdy zachowany deskryptor (#473).
    if (streamNames.empty() && !q.isDeclaration() && (!rotation || isMemoryStream(q))) continue;
    if (!streamNames.empty() && std::ranges::find(streamNames, q.id) == streamNames.end()) continue;
    const std::filesystem::path descFile = dir / (q.id + ".desc");
    bool exists                          = false;
    if (const auto error = checkDescriptorPath(descFile, exists); !error.message.empty()) return error;
    if (!exists) continue;

    rdb::Descriptor kept;
    if (const auto error = rdb::tryLoadDescriptorFile(descFile.string(), kept); !error.message.empty()) return error;
    const std::string keptType = configField(kept, rdb::TYPE);
    if (!keptType.empty() && !(q.isDeclaration() ? rdb::isDeclaredType(keptType) : rdb::isWritableType(keptType)))
      return {.cls     = rdb::ErrorClass::Plan,
              .message = std::format("storage: unsupported storage type '{}' in '{}' for stream '{}'", keptType,
                                     descFile.string(), q.id)};
    if (keptType == "TEXTSOURCE")
      if (const auto error = checkTextFields(kept, q.id); !error.message.empty()) return error;
    // Kierunek jak w rdb::verifyDescriptorMatch (plan == plik): Descriptor::operator== jest asymetryczny.
    const rdb::Descriptor planned = q.descriptorStorage();
    if (planned != kept)
      return {.cls = rdb::ErrorClass::Plan, .message = "storage: descriptor schema mismatch in '" + descFile.string() + "'"};
    if (!q.isDeclaration()) {
      // Plan nie daje SELECT-owi REF, wiec REF w jego `.desc` przenioslby zapis wyniku w miejsce,
      // ktorego plan nie zna (#278). rdb::storage odmowi tego samego, ale dopiero po starcie.
      if (const std::string keptRef = configField(kept, rdb::REF); !keptRef.empty())
        return {.cls     = rdb::ErrorClass::Plan,
                .message = std::format(
                    "stream '{}': {} moves the stream's data file to '{}', which the plan does not do; remove {} to start "
                    "the stream afresh",
                    q.id, descFile.string(), keptRef, descFile.string())};
      continue;
    }
    if (isLegacyBinaryDescriptor(planned, kept)) continue;

    // operator== porownuje tylko sloty danych, a magazyn bierze TYPE i REF z wczytanego `.desc`.
    // Do #346 zmiana sciezki albo rodzaju zrodla w planie przechodzila wiec po cichu: plan wskazywal
    // `w.txt`, a silnik czytal `v.txt` sprzed zmiany. Teraz to odmowa przed startem.
    const std::string plannedType = configField(planned, rdb::TYPE);
    if (keptType != plannedType)
      return {
          .cls     = rdb::ErrorClass::Plan,
          .message = std::format("stream '{}': {} was written for TYPE {} and the plan declares TYPE {}; remove {} to start the "
                                 "stream afresh",
                                 q.id, descFile.string(), keptType, plannedType, descFile.string())};
    const std::string keptRef    = configField(kept, rdb::REF);
    const std::string plannedRef = configField(planned, rdb::REF);
    if (keptRef != plannedRef)
      return {
          .cls     = rdb::ErrorClass::Plan,
          .message = std::format("stream '{}': {} was written for source '{}' and the plan reads '{}'; remove {} to start the "
                                 "stream afresh",
                                 q.id, descFile.string(), keptRef, plannedRef, descFile.string())};
  }
  return {};
}

rdb::Refusal checkDeclaredSources(const qTree &plan, const std::vector<std::string> &streamNames) {
  for (const auto &q : plan) {
    if (q.isCompilerDirective() || !q.isDeclaration()) continue;
    if (!streamNames.empty() && std::ranges::find(streamNames, q.id) == streamNames.end()) continue;

    const auto descriptor  = query(q).descriptorStorage();
    const std::string type = configField(descriptor, rdb::TYPE);
    if (type == "TEXTSOURCE")
      if (const auto error = checkTextFields(descriptor, q.id); !error.message.empty()) return error;
    struct stat info{};
    if (::stat(q.filename.c_str(), &info) != 0) {
      const int err = errno;
      if (err == ENOENT || err == ENOTDIR)
        return {.cls     = rdb::ErrorClass::Environment,
                .message = std::format("stream '{}': source file does not exist: {} ({})", q.id, q.filename, std::strerror(err)),
                .err     = err};
      return {.cls     = rdb::ErrorClass::Environment,
              .message = std::format("cannot inspect source '{}' for stream '{}': {}", q.filename, q.id, std::strerror(err)),
              .err     = err};
    }
    const std::string mismatch = rdb::sourceKindMismatch(type, q.filename, info.st_mode);
    if (mismatch.empty()) {
      // Kontrola przed usuwaniem wynikow, bez probnego otwierania urzadzenia (#488).
      if (::access(q.filename.c_str(), R_OK) != 0) {
        const int err = errno;
        return {.cls     = rdb::ErrorClass::Environment,
                .message = std::format("cannot read source '{}' for stream '{}': {}", q.filename, q.id, std::strerror(err)),
                .err     = err};
      }
      continue;
    }
    if (!q.isDeprecatedFile) return {.cls = rdb::ErrorClass::Plan, .message = std::format("stream '{}': {}", q.id, mismatch)};

    // Forma przestarzala: autor napisal FILE, wiec komunikat mowi, jakie slowo wybrala regula
    // i jakie pasuje do tego, co lezy pod sciezka. Inaczej FIFO spoza /dev konczylo sie
    // odmowa nazywajaca BINFILE, ktorego w planie nie ma.
    std::error_code ec;
    const auto fileType = std::filesystem::status(q.filename, ec).type();
    std::string fit;
    if (fileType == std::filesystem::file_type::character || fileType == std::filesystem::file_type::fifo)
      fit = "DEVICE";
    else if (fileType == std::filesystem::file_type::regular)
      fit = "BINFILE or TEXTFILE";
    std::string hint = std::format("deprecated FILE resolved this path as {}", sourceKindKeyword(q.kind));
    if (!fit.empty()) hint += std::format("; declare it with {}", fit);
    return {.cls = rdb::ErrorClass::Plan, .message = std::format("stream '{}': {} ({})", q.id, mismatch, hint)};
  }
  return {};
}

CounterCheck checkRotationCounter(const qTree &plan) {
  const std::string path = planCounterPath(plan);
  if (path.empty()) return {};
  const PersistentCounter::Reading reading = PersistentCounter::read(path);
  switch (reading.state) {
    case PersistentCounter::Reading::State::Missing:
    case PersistentCounter::Reading::State::Value:
      return {};
    case PersistentCounter::Reading::State::Unreadable:
      return {.environment = true,
              .refusal     = std::format("rotation counter file '{}' cannot be read ({})", path, reading.detail)};
    case PersistentCounter::Reading::State::Invalid:
      break;
  }
  return {.refusal = std::format(
              "rotation counter file '{}' is unreadable ({}); a guessed rotation number could overwrite archives of previous "
              "sessions - repair the file (a non-negative integer), or delete it to restart rotation from 0 (existing .old<N> "
              "archives will then be overwritten)",
              path, reading.detail)};
}

std::vector<std::string> deprecatedFileWarnings(const qTree &plan, const std::vector<std::string> &streamNames) {
  std::vector<const query *> deprecated;
  for (const auto &q : plan)
    if (q.isDeprecatedFile && (streamNames.empty() || std::ranges::find(streamNames, q.id) != streamNames.end()))
      deprecated.push_back(&q);
  std::ranges::sort(deprecated, {}, &query::declarationLine);

  std::vector<std::string> retVal;
  retVal.reserve(deprecated.size());
  for (const query *q : deprecated)
    retVal.push_back(std::format("line {}: DECLARE {}: FILE '{}' is deprecated, resolved as {}", q->declarationLine, q->id,
                                 q->filename, sourceKindKeyword(q->kind)));
  return retVal;
}

std::vector<std::string> deviceTimeoutWarnings(const qTree &plan, const std::optional<double> configured,
                                               const std::vector<std::string> &streamNames) {
  std::vector<const query *> late;
  for (const auto &q : plan) {
    if (q.kind != sourceKind::device) continue;
    if (!streamNames.empty() && std::ranges::find(streamNames, q.id) == streamNames.end()) continue;
    if (effectiveDeviceTimeout(q, configured, false).seconds > boost::rational_cast<double>(q.rInterval)) late.push_back(&q);
  }
  std::ranges::sort(late, {}, &query::declarationLine);

  std::vector<std::string> retVal;
  for (const query *q : late) {
    const auto timeout = effectiveDeviceTimeout(*q, configured, false);
    retVal.push_back(
        std::format("line {}: DECLARE {}: TIMEOUT {} s ({}) is longer than the interval {} s; waiting overruns the slot",
                    q->declarationLine, q->id, timeout.seconds, timeout.origin, boost::rational_cast<double>(q->rInterval)));
  }
  return retVal;
}

std::vector<std::string> deviceTimeoutReport(const qTree &plan, const std::optional<double> configured, const bool noClock,
                                             const std::vector<std::string> &streamNames) {
  std::vector<std::string> retVal;
  for (const auto &q : plan) {
    if (q.kind != sourceKind::device) continue;
    if (!streamNames.empty() && std::ranges::find(streamNames, q.id) == streamNames.end()) continue;
    const auto timeout = effectiveDeviceTimeout(q, configured, noClock);
    retVal.push_back(std::format("DEVICE stream '{}': effective TIMEOUT {} s ({})", q.id, timeout.seconds, timeout.origin));
  }
  return retVal;
}

std::vector<std::string> planStreamNames(const qTree &plan) {
  std::vector<std::string> retVal;
  for (const auto &q : plan)
    if (!q.isCompilerDirective()) retVal.push_back(q.id);
  return retVal;
}

std::string absolutePathOf(const std::string &path) {
  if (path.empty()) return {};
  std::error_code ec;
  const auto absolute = std::filesystem::absolute(std::filesystem::path(path), ec);
  if (ec) return path;
  const auto canonical = std::filesystem::weakly_canonical(absolute, ec);
  return ec ? absolute.string() : canonical.string();
}

std::string planStorageDir(const qTree &plan, const std::string_view defaultStorageDir) {
  std::string retVal(defaultStorageDir);
  for (const auto &q : plan)
    if (q.id == ":STORAGE") retVal = q.filename;
  return retVal;
}

std::vector<std::string> planStorePaths(const qTree &plan, const std::string_view defaultStorageDir) {
  const std::string storageDir = planStorageDir(plan, defaultStorageDir);

  std::vector<std::string> retVal;
  for (const auto &q : plan) {
    if (q.isCompilerDirective()) continue;
    if (q.isDeclaration()) continue;

    if (isMemoryStream(q)) continue;

    // Ta sama regula, co w streamInstance: `FILE` zastepuje nazwe zapytania nazwa pliku.
    const std::string storageName = q.filename.empty() ? q.id : q.filename;
    std::string path              = absolutePathOf((std::filesystem::path(storageDir) / storageName).string());

    // Powtorzenie znaczy, ze plan sam w sobie kieruje dwa strumienie do jednego pliku. Roszczenie
    // tego nie rozstrzyga -- porownanie idzie wylacznie miedzy instancjami -- wiec duplikat zajalby
    // wpis w slocie i niczego nie wniosl.
    if (std::ranges::find(retVal, path) == retVal.end()) retVal.push_back(std::move(path));
  }
  return retVal;
}

rdb::Refusal checkStorageDirectory(const qTree &plan, const std::string_view defaultStorageDir) {
  if (std::ranges::none_of(plan, [](const auto &q) { return !q.isCompilerDirective(); })) return {};
  std::string dir = planStorageDir(plan, defaultStorageDir);
  if (dir.empty()) return {};
  while (dir.size() > 1 && dir.back() == std::filesystem::path::preferred_separator)
    dir.pop_back();
  struct stat info{};
  if (::stat(dir.c_str(), &info) != 0) {
    const int err = errno;
    if (err == ENOENT)
      return {.cls     = rdb::ErrorClass::Plan,
              .message = std::format("storage: directory '{}' from the STORAGE directive does not exist ({}); "
                                     "RetractorDB does not create it - run 'mkdir -p {}' first",
                                     dir, absolutePathOf(dir), dir),
              .err     = err};
    return {
        .cls     = err == ENOTDIR ? rdb::ErrorClass::Plan : rdb::ErrorClass::Environment,
        .message = std::format("storage: cannot inspect directory '{}' from the STORAGE directive: {}", dir, std::strerror(err)),
        .err     = err};
  }
  if (!S_ISDIR(info.st_mode))
    return {.cls     = rdb::ErrorClass::Plan,
            .message = std::format("storage: path '{}' from the STORAGE directive exists but is not a directory", dir),
            .err     = ENOTDIR};
  return {};
}

rdb::Refusal checkPlanStorage(qTree &plan, const std::string_view defaultStorageDir,
                              const std::vector<std::string> &streamNames) {
  if (const auto error = checkStorageDirectory(plan, defaultStorageDir); !error.message.empty()) return error;
  if (const auto error = checkKeptStores(plan, defaultStorageDir, streamNames); !error.message.empty()) return error;
  if (const auto error = checkDescriptorFiles(plan, defaultStorageDir, streamNames); !error.message.empty()) return error;
  if (const auto error = checkDeclaredSources(plan, streamNames); !error.message.empty()) return error;
  return checkOutputFilesOpenable(plan, defaultStorageDir, streamNames);
}

rdb::Refusal checkOutputFilesOpenable(const qTree &plan, const std::string_view defaultStorageDir,
                                      const std::vector<std::string> &streamNames) {
  const std::filesystem::path dir = planStorageDir(plan, defaultStorageDir);
  for (const auto &q : plan) {
    if (q.isCompilerDirective() || q.isDeclaration() || isMemoryStream(q)) continue;
    if (!streamNames.empty() && std::ranges::find(streamNames, q.id) == streamNames.end()) continue;

    std::string path       = (dir / (q.filename.empty() ? q.id : q.filename)).string();
    const std::string type = q.storageType();
    if ((type == "DEFAULT" || type == "DIRECT") && !q.retention.noRetention()) path += "_segment_0";

    const auto check = [&](const std::string &file) -> rdb::Refusal {
      // Walidacja nie tworzy pliku: na nosniku embedded bylby to zapis przy
      // kazdym resecie. Istniejacy plik otwieramy tak jak akcesor, a przy nowym
      // sprawdzamy katalog, w ktorym akcesor dopiero go utworzy.
      const int fd = ::open(file.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
      if (fd >= 0) {
        struct stat info{};
        const int statResult  = ::fstat(fd, &info);
        const int statErr     = errno;
        const int closeResult = ::close(fd);
        const int closeErr    = errno;
        if (statResult != 0)
          return {
              .cls     = rdb::ErrorClass::Environment,
              .message = std::format("cannot inspect output file '{}' for stream '{}': {}", file, q.id, std::strerror(statErr)),
              .err     = statErr};
        if (!S_ISREG(info.st_mode))
          return {.cls     = rdb::ErrorClass::Plan,
                  .message = std::format("output file '{}' for stream '{}' is not a regular file", file, q.id)};
        if (closeResult != 0)
          return {
              .cls     = rdb::ErrorClass::Environment,
              .message = std::format("cannot close output file '{}' for stream '{}': {}", file, q.id, std::strerror(closeErr)),
              .err     = closeErr};
        return {};
      }
      const int openErr = errno;
      if (openErr != ENOENT)
        return {.cls     = openErr == EISDIR || openErr == ENOTDIR ? rdb::ErrorClass::Plan : rdb::ErrorClass::Environment,
                .message = std::format("cannot open output file '{}' for stream '{}': {}", file, q.id, std::strerror(openErr)),
                .err     = openErr};

      const std::filesystem::path parent = std::filesystem::path(file).parent_path().empty()
                                               ? std::filesystem::path(".")
                                               : std::filesystem::path(file).parent_path();
      struct stat parentInfo{};
      if (::stat(parent.c_str(), &parentInfo) != 0) {
        const int err = errno;
        return {.cls     = err == ENOENT || err == ENOTDIR ? rdb::ErrorClass::Plan : rdb::ErrorClass::Environment,
                .message = std::format("cannot open output file '{}' for stream '{}': parent directory '{}' is unavailable: {}",
                                       file, q.id, parent.string(), std::strerror(err)),
                .err     = err};
      }
      if (!S_ISDIR(parentInfo.st_mode))
        return {.cls     = rdb::ErrorClass::Plan,
                .message = std::format("cannot open output file '{}' for stream '{}': parent '{}' is not a directory", file,
                                       q.id, parent.string()),
                .err     = ENOTDIR};
      if (::access(parent.c_str(), W_OK | X_OK) != 0) {
        const int err = errno;
        return {.cls     = rdb::ErrorClass::Environment,
                .message = std::format("cannot create output file '{}' for stream '{}': {}", file, q.id, std::strerror(err)),
                .err     = err};
      }
      return {};
    };

    if (const auto error = check(path); !error.message.empty()) return error;
    if (type == "DEFAULT" || type == "POSIXSHD")
      if (const auto error = check(path + ".shadow"); !error.message.empty()) return error;
  }
  return {};
}

std::string planCounterPath(const qTree &plan) {
  for (const auto &q : plan)
    if (q.id == ":ROTATION" && !q.filename.empty()) return absolutePathOf(q.filename);
  return {};
}
