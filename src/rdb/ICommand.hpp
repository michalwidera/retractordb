#pragma once
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "rdb/storage.hpp"
#include "xtrdbTypes.hpp"

struct CommandContext {
  std::unique_ptr<rdb::storage> &dacc;
  payloadStatusType &payloadStatus;
  std::string &file;
  std::string &storageParam;
  std::string &storagePolicy;
  const Colors &colors;
  bool &rox;
};

class ICommand {
 public:
  virtual ~ICommand()                                                                  = default;
  [[nodiscard]] virtual std::pair<std::string, std::vector<std::string>> usage() const = 0;
  [[nodiscard]] virtual bool requiresConnection() const { return true; }
  // true → print "ok"; false → skip ok (continue)
  virtual bool execute(CommandContext &ctx) = 0;
};
