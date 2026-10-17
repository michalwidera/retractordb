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

/// Funkcja checkNamespace powinna:
/// - odrzucać pustą nazwę i nazwę zawierającą '/' wyjątkiem Rejected.
///   Test: `IpcSecurity.InvalidNamesMissingObjectsAndExclusiveMemoryCreation`.
/// - sprawdzać każdy katalog ścieżki obiektu: wpis i jego cel należą do roota albo do bieżącego efektywnego UID, cel jest katalogiem, a prawo zapisu dla grupy lub innych idzie w parze z bitem sticky; niezgodność zgłaszać jako Rejected.
///   Test: `IpcSecurity.ForeignDirectoryCannotAuthorizeAnObject`, `IpcSecurity.NamespaceRejectsWritableNonstickyAndNondirectoryTargets`.
/// - przy createDirectories tworzyć brakujący katalog z trybem kDirectoryPermissions i sprawdzać go jak istniejący.
///   Nietestowalne: na backendzie POSIX katalog /dev/shm juz istnieje. Tworzenie brakujacych katalogow backendu plikowego nie jest tu wykonane; kontrola strukturalna checkNamespace w src/common/ipcSecurity.cpp (mkdir z kDirectoryPermissions, ponowny lstat i stat). Darwin wymaga osobnej weryfikacji.
/// - bez createDirectories zgłaszać brak katalogu jako interprocess_exception z ENOENT (chwilowy brak), a inny błąd odczytu katalogu jako Rejected.
///   Test: `IpcSecurity.NamespaceDistinguishesMissingDirectoryFromInspectionFailure`.
///
/// @note Katalogi nazwy musza chronic wpisy przed podmiana przez inne konto; katalog obcego konta daje mu prawo
///       podmiany wpisow mimo sticky. createDirectories ma znaczenie tylko dla plikowego backendu Boosta.
void checkNamespace(const std::string &name, bool createDirectories = false);

/// Funkcja checkOpened powinna:
/// - przed mapowaniem sprawdzać przestrzeń nazw (checkNamespace) i deskryptor otwartego obiektu: właściciel równy geteuid(), zwykły plik z jednym dowiązaniem, tryb dokładnie kObjectPermissions (0600) i nazwa nadal wskazująca ten sam obiekt.
///   Test: `IpcSecurity.OpenChecksTheDescriptorOwner`, `IpcSecurity.OpenRejectsBroadPermissionsBeforeReadingQueueLayout`, `IpcSecurity.HardlinkAndSymlinkObjectsAreRejectedWithoutDamagingTheirTarget`, `IpcSecurity.OpenedDescriptorMustStillBeTheNamedObject`.
/// - każdą niezgodność zgłaszać wyjątkiem Rejected z diagnostyką ERROR zawierającą UID właściciela i oczekiwany UID.
///   Test: `IpcSecurity.OpenChecksTheDescriptorOwner`. Nietestowalne: wspolna diagnostyka pozostalych odmow jest sprawdzona strukturalnie przez reject w src/common/ipcSecurity.cpp; nie kazda odmowa ma osobna asercje logu.
void checkOpened(const boost::interprocess::shared_memory_object &object, const std::string &name);

/// Funkcja createMemory powinna:
/// - przygotowywać katalogi przez checkNamespace(name, true), tworzyć obiekt pamięci dzielonej wyłącznie jako nowy, z trybem kObjectPermissions, i sprawdzać go przez checkOpened.
///   Test: `IpcSecurity.NewObjectsHaveMode0600AndExpectedOwner`, `IpcSecurity.InvalidNamesMissingObjectsAndExclusiveMemoryCreation`.
/// - każde niepowodzenie tworzenia, także istniejącą nazwę, zgłaszać jako Rejected z UID właściciela znalezionego wpisu.
///   Test: `IpcSecurity.InvalidNamesMissingObjectsAndExclusiveMemoryCreation`. Nietestowalne: pozostale bledy tworzenia sa przekazywane przez wspolne creationFailed/reject; kontrola strukturalna src/common/ipcSecurity.cpp, bez wyczerpywania zasobow hosta.
boost::interprocess::shared_memory_object createMemory(const std::string &name);

/// Funkcja openMemory powinna:
/// - przed otwarciem sprawdzać przestrzeń nazw i metadane istniejącego wpisu bez podążania za dowiązaniem, a po otwarciu w trybie mode sprawdzać deskryptor przez checkOpened.
///   Test: `IpcSecurity.OpenChecksTheDescriptorOwner`, `IpcSecurity.HardlinkAndSymlinkObjectsAreRejectedWithoutDamagingTheirTarget`, `IpcSecurity.OpenedDescriptorMustStillBeTheNamedObject`.
/// - brak obiektu zgłaszać wyjątkiem interprocess_exception Boosta, a obiekt niezaufany jako Rejected.
///   Test: `IpcSecurity.InvalidNamesMissingObjectsAndExclusiveMemoryCreation`, `IpcSecurity.OpenRejectsBroadPermissionsBeforeReadingQueueLayout`.
///
/// @note Wpis sprawdzany osobno, bo Boost w backendzie plikowym otwiera dowiazania.
boost::interprocess::shared_memory_object openMemory(const std::string &name, boost::interprocess::mode_t mode);

/// Funkcja createQueue powinna:
/// - przygotowywać katalogi przez checkNamespace(name, true), tworzyć kolejkę komunikatów wyłącznie jako nową, o podanej liczbie elementów i rozmiarze komunikatu, z trybem kObjectPermissions, i sprawdzać jej obiekt przez openMemory tylko do odczytu.
///   Test: `IpcSecurity.NewObjectsHaveMode0600AndExpectedOwner`, `IpcSecurity.ExclusiveCollisionPreservesQueueAndItsContents`, `IpcSecurity.ResubscriptionRecreatesOwnQueueWithRequestedCapacity`.
/// - każde niepowodzenie tworzenia zgłaszać jako Rejected.
///   Test: `IpcSecurity.ExclusiveCollisionPreservesQueueAndItsContents`.
std::unique_ptr<boost::interprocess::message_queue> createQueue(const std::string &name, std::size_t elements,
                                                                std::size_t messageSize);

/// Funkcja openQueue powinna:
/// - przed otwarciem kolejki sprawdzać jej obiekt przez openMemory tylko do odczytu, a po otwarciu potwierdzać, że nazwa nadal wskazuje sprawdzony obiekt, i niezgodność zgłaszać jako Rejected.
///   Test: `IpcSecurity.QueueOpenRejectsNameReplacementAfterOpening`, `IpcSecurity.OpenRejectsBroadPermissionsBeforeReadingQueueLayout`.
///
/// @note Kolejka Boosta nie udostepnia deskryptora, wiec sprawdzany jest jej obiekt otwarty bez mapowania.
std::unique_ptr<boost::interprocess::message_queue> openQueue(const std::string &name);

/// Funkcja removeOwned powinna:
/// - zwracać true, gdy obiekt nie istnieje albo został usunięty.
///   Test: `IpcSecurity.InvalidNamesMissingObjectsAndExclusiveMemoryCreation`.
/// - usuwać wyłącznie zwykły obiekt z jednym dowiązaniem, należący do bieżącego efektywnego UID, w zaufanej przestrzeni nazw, niezależnie od jego trybu i bez mapowania ani czytania treści.
///   Test: `IpcSecurity.SubscriptionRefusalAndCleanupPreserveForeignQueue`, `IpcSecurity.OpenRejectsBroadPermissionsBeforeReadingQueueLayout`, `IpcSecurity.HardlinkAndSymlinkObjectsAreRejectedWithoutDamagingTheirTarget`, `IpcSecurity.ForeignDirectoryCannotAuthorizeAnObject`; integracja `it_ipc_security-foreign-uid` wymaga root i moze byc niewykonana.
/// - przy odmowie albo błędzie logować diagnostykę ERROR i zwracać false, nie zgłaszając wyjątku.
///   Test: `IpcSecurity.CleanupIoFailureLogsAndReturnsFalseWithoutRemovingTheObject`, `IpcSecurity.SubscriptionRefusalAndCleanupPreserveForeignQueue`.
///
/// @note Wolac wylacznie pod blokada tozsamosci/obecnosci albo dla wlasnej subskrypcji: blokada wolajacego
///       wyklucza zywy serwer.
bool removeOwned(const std::string &name) noexcept;

}  // namespace ipc::security
