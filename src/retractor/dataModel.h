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
  struct fuseCabinet {
    /**
     * Mapa zawierająca informacje powiązane z wewnętrznym deskryptorem
     * domyślinie - każdy rekord jest świeży i niegotowy == false - Not Ready To
     * Check In jeśli jest wyliczony i gotowy do zapisu to == true - Ready To
     * Check In
     */
    std::vector<bool> publicDescriptorFuses;

    fuseCabinet(int capacity) {
      publicDescriptorFuses.resize(capacity);
      reset();
    };
    void reset() {
      for (auto i : publicDescriptorFuses) i = true;
    };
    void blown(int position) { publicDescriptorFuses[position] = false; };
    bool is_good(int position) { return publicDescriptorFuses[position]; };
    bool has_any_good() {
      for (auto i : publicDescriptorFuses)
        if (i) return true;
      return false;
    };
  };
  std::unique_ptr<rdb::DataStorageAccessor<std::byte>> uPtr_storage;
  std::unique_ptr<std::byte[]> uPtr_payload;
  std::unique_ptr<fuseCabinet> uPtr_fuseset;
  rdb::Descriptor internalDescriptor;

  streamInstance(const std::string file, const rdb::Descriptor publicDescriptor,
                 const rdb::Descriptor internalDescriptor)
      : internalDescriptor(internalDescriptor) {
    // Ta sekwencja utworzy plik danych i deskrypor
    uPtr_storage.reset(new rdb::DataStorageAccessor(publicDescriptor, file));
    uPtr_payload.reset(new std::byte[uPtr_storage->getDescriptor().GetSize()]);
    uPtr_fuseset.reset(new fuseCabinet(publicDescriptor.size()));
  };

  /**
   *  Dane występujące w publicznym schemacie są pobierane przez zewnętrzne
   * zapytania i zapisywane jednokrotnie przez wewnętrzne. czyli wiele getPublic
   * z zewnątrz i tylko jedno wywołanie setPublic z wewnętrznego wyliczenia
   *  setPublic powinno ustawić w publicDescriptorFuses wartość true - tzn.
   * Ready To Check In
   */
  std::any getPublic(int position) {}
  void setPublic(int position, std::any value) {
    uPtr_fuseset->blown(position);
    if (!uPtr_fuseset->has_any_good()) {
      /**
       * zapis danych do strumienia powinnien się udać tylko jeśli
       * wszystkie elementy publicznego deskryptora zostały zatwierdzone
       * tzn. są w stanie Ready To Check In
       * uwaga: Istnieje opcja że ta funkcja zostanie zredukowana (usunięta)
       * automatycznie po wszystkich setPublic - nastąpi flushDataToStorage
       */
      uPtr_fuseset->reset();
    }
  }

  /**
   * Dane z wewnętrznego schematu służą do wyznaczenia publicznego obrazu
   */
  std::any getInternal(int position);
  void setInternal(int position, std::any value);  // czy potrzebne ?
};
struct dataModel : public std::map<std::string, streamInstance> {};