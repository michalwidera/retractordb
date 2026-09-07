#include "planSource.hpp"

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <system_error>

#include <spdlog/spdlog.h>

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

void dropStalePlanArtifacts(qTree &plan, const compiler &cm, const std::vector<std::pair<std::string, std::string>> &lines) {
  if (std::ranges::any_of(plan, [](const auto &it) { return it.id == ":ROTATION"; })) return;

  std::string storage_location;
  for (const auto &it : plan)
    if (it.id == ":STORAGE") {
      storage_location = it.filename;
    }

  // Nazwa zwracana przez parser jest nazwa Z ZAPISU, a ta nie musi byc nazwa zapytania
  // w planie: generator `STREAM cell[24]` daje jedna linie RQL i 24 strumienie `cell$0`..
  // `cell$23`, a samego `cell` w planie nie ma. Rodziny bierzemy z kompilatora, bo to
  // jedyne pewne zrodlo — patrz compiler::generatedStreams().
  const auto &generatedStreams = cm.generatedStreams();
  for (const auto &[stream_id, query_text] : lines) {
    if (stream_id.empty()) continue;

    const auto family = generatedStreams.find(stream_id);
    const std::vector<std::string> definedStreams =
        (family != generatedStreams.end()) ? family->second : std::vector<std::string>{stream_id};

    for (const auto &defined_id : definedStreams) {
      if (plan[defined_id].isDeclaration()) continue;
      if (plan[defined_id].isCompilerDirective()) continue;
      dropArtifactFile(std::filesystem::path(storage_location) / defined_id);
      dropArtifactFile(std::filesystem::path(storage_location) / (defined_id + ".desc"));
      dropArtifactFile(std::filesystem::path(storage_location) / (defined_id + ".meta"));
    }
  }
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

    // Typ magazynu rozstrzygamy DOKLADNIE tak, jak zrobi to wykonanie: `VOLATILE` wpisuje
    // `TYPE` do deskryptora (query::descriptorStorage), a ten w rdb::storage::attachStorage
    // wygrywa z polityka z klauzuli `STORAGE`.
    const std::string storageType = (q.policy.second != 0) ? q.policy.first : q.storage_policy;
    if (storageType == "MEMORY") continue;

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
