#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

namespace ipc::security {

/// Odmowa zaufania obiektowi, odrozniona od chwilowego braku kolejki klienta.
/// @requirements-exempt Rejected: Znacznik kategorii odmowy; zachowanie wyjatku dziedziczy ze std::runtime_error.
class Rejected : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// Katalogi nazwy musza chronic wpisy przed podmiana przez inne konto.
/// createDirectories dotyczy tylko plikowego backendu Boosta.
void checkNamespace(const std::string &name, bool createDirectories = false);

/// Sprawdza deskryptor PRZED mapowaniem: zwykly plik, geteuid(), tryb 0600 i jedna nazwa.
void checkOpened(const boost::interprocess::shared_memory_object &object, const std::string &name);

boost::interprocess::shared_memory_object createMemory(const std::string &name);
boost::interprocess::shared_memory_object openMemory(const std::string &name, boost::interprocess::mode_t mode);
std::unique_ptr<boost::interprocess::message_queue> createQueue(const std::string &name, std::size_t elements,
                                                                std::size_t messageSize);
std::unique_ptr<boost::interprocess::message_queue> openQueue(const std::string &name);

/// Wolac wylacznie pod blokada tozsamosci/obecnosci albo dla wlasnej subskrypcji.
/// Pozwala sprzatnac stary tryb wlasnego obiektu, lecz nigdy obcy UID lub dowiazanie.
/// Brak obiektu jest sukcesem. Odmowa/blad ma diagnostyke ERROR i zwraca falsz.
bool removeOwned(const std::string &name) noexcept;

}  // namespace ipc::security
