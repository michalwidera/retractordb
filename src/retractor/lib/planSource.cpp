#include "planSource.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <sstream>
#include <system_error>

#include <spdlog/spdlog.h>

#include "rdb/descriptorIO.hpp"
#include "rdb/storageShadow.hpp"
#include "RQLParser.hpp"

namespace {

void dropArtifactFile(const std::filesystem::path &artifact_filename) {
  if (std::filesystem::exists(artifact_filename)) {
    std::error_code ec;
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
  std::error_code ec;
  for (const auto &entry : std::filesystem::directory_iterator(parent, ec)) {
    const std::string name = entry.path().filename().string();
    std::string_view rest  = name;
    if (!rest.starts_with(prefix)) continue;
    rest.remove_prefix(prefix.size());
    if (rest.ends_with(".shadow")) rest.remove_suffix(std::string_view(".shadow").size());
    if (!rest.empty() && std::ranges::all_of(rest, [](unsigned char c) { return std::isdigit(c) != 0; }))
      dropArtifactFile(entry.path());
  }
}

/// Konfiguracja magazynu w postaci, w jakiej operator zapisalby ja w planie.
std::string describeStore(const std::string &type, const rdb::retention_t &retention) {
  if (retention.noRetention()) return std::format("STORAGE {} without RETENTION", type);
  return std::format("STORAGE {} RETENTION {} {}", type, retention.capacity, retention.segments);
}

}  // namespace

PlanSource parsePlanText(qTree &plan, const std::string &text) {
  PlanSource retVal;
  std::istringstream source(text);
  // Numer wiersza idzie do parsera, bo tylko tutaj wiadomo, w ktorym miejscu pliku stoi
  // instrukcja: parser dostaje ja wyjeta z kontekstu i sam liczylby od jedynki.
  std::vector<std::string> statementKeywords;
  for (const auto &[stmt, firstLine] : readLogicalLines(source)) {
    auto [status, first_keyword, stream_name] = parserRQLString(plan, stmt, statementKeywords, firstLine);
    if (status != "OK") {
      retVal.status = status;
      return retVal;
    }
    retVal.lines.emplace_back(stream_name, stmt);
  }
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
    if (q.isDeclaration() || q.isCompilerDirective()) continue;
    if (rotation && !isMemoryStream(q)) continue;
    dropArtifactFamily(dir, q);
  }
}

std::string checkKeptStores(qTree &plan, const std::string_view defaultStorageDir) {
  if (std::ranges::none_of(plan, [](const auto &it) { return it.id == ":ROTATION"; })) return {"OK"};
  const std::filesystem::path dir = planStorageDir(plan, defaultStorageDir);
  for (auto &q : plan) {
    if (q.isDeclaration() || q.isCompilerDirective() || isMemoryStream(q)) continue;
    const std::filesystem::path descFile = dir / (q.id + ".desc");
    if (!std::filesystem::exists(descFile)) continue;

    // Ta sama regula co rdb::storage::attachStorage: TYPE z wczytanego .desc wygrywa, bez niego
    // obowiazuje STORAGE z planu.
    rdb::Descriptor kept    = rdb::loadDescriptorFile(descFile.string());
    const auto keptType     = kept.storagePolicy().first.empty() ? q.storage_policy : kept.storagePolicy().first;
    const auto keptStore    = describeStore(keptType, kept.retention());
    const auto plannedStore = describeStore(q.storageType(), q.descriptorStorage().retention());
    if (keptStore == plannedStore) continue;
    return std::format(
        "Stream '{}' keeps its files under ROTATION, but {} was written with {} and the plan asks for {}: "
        "put {} back in the plan, or remove the stream's files to start it afresh",
        q.id, descFile.string(), keptStore, plannedStore, keptStore);
  }
  return {"OK"};
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

std::string planCounterPath(const qTree &plan) {
  for (const auto &q : plan)
    if (q.id == ":ROTATION" && !q.filename.empty()) return absolutePathOf(q.filename);
  return {};
}
