#include "rdb/descriptorIO.hpp"

#include <fcntl.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <span>
#include <sstream>
#include <utility>

#include <spdlog/spdlog.h>

#include "rdb/error.hpp"
#include "rdb/storageFile.hpp"

extern std::string parserDESCString(rdb::Descriptor &desc, std::string_view inlet);

namespace rdb {

Refusal tryLoadDescriptorFile(const std::string &descriptorFile, Descriptor &descriptor) {
  // `.desc` lezy w katalogu magazynu: dowiazanie pod jego nazwa jest odmowa (#374).
  const StorageFd file(descriptorFile, O_RDONLY);
  if (!file.isOpen()) {
    const int openErrno = errno;  // przed skladaniem napisu - alokacja moze ruszyc errno
    return {.cls     = ErrorClass::Environment,
            .message = "cannot open descriptor file: " + descriptorFile + ": " + std::strerror(openErrno),
            .err     = openErrno};
  }

  const off_t fileSize = file.size();
  if (fileSize < 0) {
    const int err = errno;
    return {.cls     = ErrorClass::Environment,
            .message = "cannot inspect descriptor file: " + descriptorFile + ": " + std::strerror(err),
            .err     = err};
  }
  std::string content(fileSize > 0 ? static_cast<size_t>(fileSize) : 0, '\0');
  const ssize_t got = file.readAt(std::as_writable_bytes(std::span{content}), 0);
  if (got < 0) {
    const int err = errno;
    return {.cls     = ErrorClass::Environment,
            .message = "cannot read descriptor file: " + descriptorFile + ": " + std::strerror(err),
            .err     = err};
  }
  content.resize(static_cast<size_t>(got));

  Descriptor parsed;
  if (const std::string result = parserDESCString(parsed, content); result != "OK")
    return {.cls = ErrorClass::Plan, .message = "descriptor parse failed in '" + descriptorFile + "': " + result};
  if (parsed.getSizeInBytes() == 0)
    return {.cls = ErrorClass::Plan, .message = "storage: empty descriptor file: " + descriptorFile};
  descriptor = std::move(parsed);
  return {};
}

Result<Descriptor> loadDescriptorFile(const std::string &descriptorFile) {
  Descriptor descriptor;
  if (Refusal refusal = tryLoadDescriptorFile(descriptorFile, descriptor); !refusal.message.empty())
    return fail(refusal.cls == ErrorClass::Environment ? Errc::IO : Errc::CorruptDescriptor, std::move(refusal.message),
                refusal.err);
  return descriptor;
}

Result<> saveDescriptorFile(const std::string &descriptorFile, const Descriptor &descriptor) {
  std::ostringstream content;
  content << descriptor;
  const std::string text = content.str();
  // Bez podazania za dowiazaniem: O_TRUNC przez dowiazanie obcialby jego cel (#374).
  const StorageFd descFile(descriptorFile, O_WRONLY | O_CREAT | O_TRUNC);
  if (!descFile.isOpen()) {
    const int err = errno;
    return fail(Errc::IO,
                fmt::format("storage: failed to open descriptor file for writing: {}: {}", descriptorFile, std::strerror(err)),
                err);
  }
  if (!descFile.write(std::as_bytes(std::span{text}))) {
    const int err = errno != 0 ? errno : EIO;
    return fail(Errc::IO, fmt::format("storage: failed to write descriptor file: {}", descriptorFile), err);
  }
  return {};
}

Result<> verifyDescriptorMatch(const Descriptor &provided, const Descriptor &existing, const std::string &descriptorFile) {
  if (provided == existing) return {};

  SPDLOG_ERROR("Descriptors do not match.");
  std::cerr << "Error in data descriptor file: " << descriptorFile << '\n';
  std::cerr << "Provided Descriptor:\n" << provided << "\nExisting Descriptor:\n" << existing << '\n';
  // K1 po preflight (#473): checkDescriptorFiles porownuje plan == plik przed publikacja,
  // takze dla nowych SELECT-ow ad-hoc. Start/reset kasuja niezachowywane deskryptory.
  return fail(Errc::Logic, "storage: descriptor schema mismatch - remove data files and restart");
}

}  // namespace rdb
