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

struct streamComposite {
  enum { noHexFormat = false, HexFormat = true };
  std::unique_ptr<std::byte[]> payload;
  std::unique_ptr<rdb::payLoadAccessor<std::byte>> accessor;
  std::any get(int position) { return accessor->get_item(position); };
  void set(int position, std::any value) {
    accessor->set_item(position, value);
  };
  streamComposite(rdb::Descriptor descriptor) {
    payload.reset(new std::byte[descriptor.GetSize()]);
    accessor.reset(new rdb::payLoadAccessor(descriptor, payload.get(), noHexFormat));
  };
};

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
  std::unique_ptr<rdb::DataStorageAccessor<std::byte>> storage;

  std::unique_ptr<streamComposite> publicSchema;
  std::unique_ptr<streamComposite> internalSchema;

  streamInstance(const std::string file);
};

struct dataModel : public std::map<std::string, streamInstance> {};
