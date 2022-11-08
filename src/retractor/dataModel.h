#pragma once

#include <rdb/desc.h>   // rdb::Descriptor
#include <rdb/dsacc.h>  // rdb::DataStorageAccessor
#include <spdlog/spdlog.h>

#include <any>
#include <cassert>
#include <map>
#include <memory>  // unique_ptr
#include <string>
#include <vector>

#include "QStruct.h"  // number

/**
 * Ta klasa ma za zadnie przedstawić warstwę abstrakcji danych.
 * Problem polegał na tym że poszególne strumienie i zapytania muszą inaczej
 * interpretować nazwy kolumn.
 * I tak select a[0],a[2] stream a from b+c
 * zakładając że strumienie b i c tworzą co najmnie trzelementowy zbiór to
 * to zapytanie tworzy strumień a o elementach a[0],a[1] ale z elementów sumy
 * schematów strumieni b i c z klauzuli select (wewnątrz zapytania) widać 0,1,2
 * a z zewnątrz (z innych zapytań) tylko 0 i 1
 *
 * TODO: Translate to english afterall - before merge to Master
 */
struct streamInstance {
  std::unique_ptr<rdb::DataStorageAccessor<std::byte>> uPtr_storage;
  std::unique_ptr<std::byte[]> uPtr_payload;
  rdb::Descriptor internalDescriptor;

  streamInstance(const std::string file, const rdb::Descriptor publicDescriptor,
                 const rdb::Descriptor internalDescriptor)
      : internalDescriptor(internalDescriptor) {
    // Ta sekwencja utworzy plik danych i deskrypor
    uPtr_storage.reset(new rdb::DataStorageAccessor(publicDescriptor, file));
    uPtr_payload.reset(new std::byte[uPtr_storage->getDescriptor().GetSize()]);
  };

  /**
   * Mapa zawierająca informacje powiązane z wewnętrznym deskryptorem
   * domyślinie - każdy rekord jest świeży i niegotowy == false - Not Ready To
   * Check In jeśli jest wyliczony i gotowy do zapisu to == true - Ready To
   * Check In
   */
  std::vector<bool> publicDescriptorStatus;

  /**
   * zapis danych do strumienia powinnien się udać tylko jeśli
   * wszystkie elementy publicznego deskryptora zostały zatwierdzone
   * tzn. są w stanie Ready To Check In
   * uwaga: Istnieje opcja że ta funkcja zostanie zredukowana (usunięta)
   * automatycznie po wszystkich setPublic - nastąpi flushDataToStorage
   */
  bool flushDataToStorage();

  /**
   *  Dane występujące w publicznym schemacie są pobierane przez zewnętrzne
   * zapytania i zapisywane jednokrotnie przez wewnętrzne. czyli wiele getPublic
   * z zewnątrz i tylko jedno wywołanie setPublic z wewnętrznego wyliczenia
   *  setPublic powinno ustawić w publicDescriptorStatusSet wartość true - tzn.
   * Ready To Check In
   */
  std::any getPublic(int position);
  void setPublic(int position, std::any value);

  /**
   * Dane z wewnętrznego schematu służą do wyznaczenia publicznego obrazu
   * strumienia dlatego tylko występue getInternal - bez setInternal
   */
  number getInternal(int position);
};
struct dataModel : public std::map<std::string, streamInstance> {};