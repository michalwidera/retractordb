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
/// - [xtrdb.command.usage~1] definiować dla każdej komendy xtrdb opis składni i pomocy przez usage, zwracający nazwę z argumentami oraz linie opisu; pusta nazwa pomija komendę w pomocy.
/// - [xtrdb.command.requires-connection~1] przez requiresConnection deklarować, czy komenda wymaga otwartego magazynu; domyślnie prawda, a launcher nie wywołuje takiej komendy bez magazynu.
/// - [xtrdb.command.execute-result~1] wymagać od implementacji execute wykonania komendy na stanie przekazanym w CommandContext i zwrócenia prawdy, gdy launcher ma potwierdzić wykonanie, albo fałszu, gdy potwierdzenie ma zostać pominięte.
/// - [xtrdb.command.error-reporting~1] wymagać od implementacji zgłaszania błędów magazynu komunikatem dla operatora bez kończenia procesu i bez pozostawiania w kontekście magazynu, którego nie udało się otworzyć.
///   Ograniczenie: naruszenie niezmiennika silnika nadal moze zakonczyc proces; kontrakt dotyczy odmow operacji.
/// - [xtrdb.command.virtual-dtor~1] umożliwiać niszczenie implementacji przez wskaźnik na ICommand.
class ICommand {
 public:
  virtual ~ICommand()                                                                  = default;
  [[nodiscard]] virtual std::pair<std::string, std::vector<std::string>> usage() const = 0;
  [[nodiscard]] virtual bool requiresConnection() const { return true; }
  // true → print "ok"; false → skip ok (continue)
  virtual bool execute(CommandContext &ctx) = 0;
};
