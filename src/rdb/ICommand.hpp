#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "rdb/storage.hpp"
#include "xtrdbTypes.hpp"

/// @requirements-exempt CommandContext: Zestaw referencji do stanu przekazywanego obiektom komend.
struct CommandContext {
  std::unique_ptr<rdb::storage> &dacc;
  payloadStatusType &payloadStatus;
  std::string &file;
  std::string &storageParam;
  std::string &storagePolicy;
  const Colors &colors;
  bool &rox;
  const std::vector<std::string> &refDirs;  ///< storage.ref_dirs z retractor.toml (#278)
};

/// Obiekt klasy ICommand powinien:
/// - definiować dla każdej komendy xtrdb opis składni i pomocy przez usage, zwracający nazwę z argumentami oraz linie opisu; pusta nazwa pomija komendę w pomocy.
///   Test: `it_issue153_storagemap_meta_cases-contracts`.
/// - przez requiresConnection deklarować, czy komenda wymaga otwartego magazynu; domyślnie prawda, a launcher nie wywołuje takiej komendy bez magazynu.
///   Test: `it_issue153_storagemap_meta_cases-contracts`.
/// - wymagać od implementacji execute wykonania komendy na stanie przekazanym w CommandContext i zwrócenia prawdy, gdy launcher ma potwierdzić wykonanie, albo fałszu, gdy potwierdzenie ma zostać pominięte.
///   Test: `it_issue153_storagemap_meta_cases-contracts`.
/// - wymagać od implementacji zgłaszania błędów magazynu komunikatem dla operatora bez kończenia procesu i bez pozostawiania w kontekście magazynu, którego nie udało się otworzyć.
///   Test: `it_issue153_storagemap_meta_cases-contracts`, `it_xtrdb_open_corrupt_meta-run`. Ograniczenie: naruszenie niezmiennika silnika nadal moze zakonczyc proces; kontrakt dotyczy odmow operacji.
/// - umożliwiać niszczenie implementacji przez wskaźnik na ICommand.
///   Sprawdzenie kompilacji: test/UnitTest/test_contracts.cpp - static_assert(std::has_virtual_destructor_v<ICommand>).
class ICommand {
 public:
  virtual ~ICommand()                                                                  = default;
  [[nodiscard]] virtual std::pair<std::string, std::vector<std::string>> usage() const = 0;
  [[nodiscard]] virtual bool requiresConnection() const { return true; }
  // true → print "ok"; false → skip ok (continue)
  virtual bool execute(CommandContext &ctx) = 0;
};
