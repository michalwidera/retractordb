#pragma once

#include <rdb/desc.h>   // rdb::Descriptor
#include <rdb/dsacc.h>  // rdb::DataStorageAccessor
#include <rdb/payloadacc.h>
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
  rdb::Descriptor internalDataDescriptor;
  rdb::Descriptor externalDataDescriptor;
  std::unique_ptr<rdb::payLoadAccessor<std::byte>> uPtr_plAcces;

  streamInstance(const std::string file);

  /**
   *  Dane występujące w publicznym schemacie są pobierane przez zewnętrzne
   * zapytania i zapisywane jednokrotnie przez wewnętrzne. czyli wiele getPublic
   * z zewnątrz i tylko jedno wywołanie setPublic z wewnętrznego wyliczenia
   *  setPublic powinno ustawić w publicDescriptorFuses wartość true - tzn.
   * Ready To Check In
   */
  std::any getPublic(int position);
  void setPublic(int position, std::any value);

  /**
   * Dane z wewnętrznego schematu służą do wyznaczenia publicznego obrazu
   * Dane z wewnętrznego schematu to nie jest kopia publicznego schematu
   * Te dane są płaskim rozwinięciem tego co wychodzi z klauzuli FROM
   * (bez obliczeń) - te dane służą do obliczeń wystawianych przez publiczny
   * obraz
   */
  std::any getInternal(int position);
  void setInternal(int position, std::any value);  // czy potrzebne ?
};

struct dataModel : public std::map<std::string, streamInstance> {};
