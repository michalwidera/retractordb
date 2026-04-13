#pragma once

#include <memory>  // std::unique_ptr
#include <string>

#include <boost/circular_buffer.hpp>

#include "descriptor.h"
#include "faccbindev.h"
#include "faccfs.h"
#include "faccmemory.h"
#include "faccposix.h"
#include "faccposixshd.h"
#include "facctxtsrc.h"
#include "fagrp.h"
#include "metaDataStream.h"
#include "payload.h"

namespace rdb {
enum class storageState { noDescriptor, openAndCreate };
enum class sourceState { empty, flux, armed };

/// @brief Definicja klasy implmentującej zarządzenie danymi zapisywanymi w bazie danych.
///
/// Obiekt storage powinien:
/// - być tworzony na podstawie identyfikatora zapytania (qryID) i nazwy pliku (fileName).
/// - umożliwiać dołączenie obiektu descriptor, który definiuje strukturę danych zarządzanych przez storage.
/// - umożliwiać odczyt i zapis danych do/z pliku, zgodnie z opisem w descriptorze.
/// - zarządzać stanem pliku danych (otwarty, zamknięty, itp.) i stanem bufora (pusty, wypełniony, itp.).
/// - umożliwiać ustawienie pojemności bufora, jeśli jest to wymagane.
/// - umożliwiać oznaczenie storage jako jednorazowego (one-shot) lub wielokrotnego użytku.
/// - umożliwiać oznaczenie storage jako "hold", co oznacza, że nie będzie przetwarzał danych, dopóki nie pojawi się pierwsze zapytanie.
/// - umożliwiać zwolnienie storage z "hold" i rozpoczęcie przetwarzania danych.
/// - umożliwiać sprawdzenie, czy plik descriptor istnieje.
/// - umożliwiać czyszczenie danych w payload.
/// - umożliwiać resetowanie stanu storage (np. dla celów testowych).
/// - umożliwiać usunięcie storage i descriptor, jeśli storage jest oznaczony jako jednorazowy (disposable).
/// - łączyć zdefiniowany sposób dostępu do pliku (np. przez FileInterface) z zarządzaniem danymi w pamięci (np. przez payload).

class storage {
  std::unique_ptr<FileInterface> accessor_;
  std::unique_ptr<rdb::payload> storagePayload_;
  std::unique_ptr<rdb::payload> chamber_;

  bool isDisposable_          = false;  // if true - storage and descriptor will be deleted after use
  bool isOneShot_             = false;  // if false - storage will be looped when end is reached
  bool isHold_                = false;  // if true - no processing until first query appear
  size_t recordsCount_        = 0;
  std::string descriptorFile_ = "";
  std::string storageFile_    = "";
  std::string metaIndexFile_  = "";  // file path for saving/loading the meta index
  std::string storageType_    = "";
  int percounter_             = -1;

  void moveRef();
  void attachStorage();

  boost::circular_buffer<rdb::payload> circularBuffer_{0};

  void abortIfStorageNotPrepared();
  void initializeAccessor();

  std::unique_ptr<rdb::metaDataStream> metaDataStream_;

 public:
  storage() = delete;
  explicit storage(const std::string_view qryID,                    //
                   const std::string_view fileName,                 //
                   const std::string_view storageParam,             //
                   const std::string_view storageType = "DEFAULT",  //
                   bool oneShot                       = false,      //
                   bool isHold                        = false,      //
                   int percounter                     = -1          //
  );
  virtual ~storage();

  Descriptor descriptor;

  storageState dataFileStatus = storageState::noDescriptor;
  sourceState bufferState     = sourceState::empty;  // ? test lock

  void attachDescriptor(const Descriptor *descriptor = nullptr);

  bool write(const size_t recordIndex = std::numeric_limits<size_t>::max());
  bool revRead(const size_t recordIndexFromBack, uint8_t *destination = nullptr);
  bool read(const size_t recordIndexFromFront, uint8_t *destination = nullptr);
  void fire();
  void purge();

  std::unique_ptr<rdb::payload>::pointer getPayload();

  void setDisposable(bool value);
  void releaseOnHold();
  size_t getRecordsCount();
  bool descriptorFileExist();

  bool isDeclared();

  void setCapacity(const int capacity);
  void cleanPayload(uint8_t *destination = nullptr);

  // technical function - for unit tests
  void reset();
};
}  // namespace rdb
