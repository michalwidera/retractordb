#pragma once

#include <rdb/desc.h>        // rdb::Descriptor
#include <rdb/dsacc.h>       // rdb::DataStorageAccessor
#include <rdb/payloadacc.h>  // rdb::payLoadAccessor

#include <any>
#include <map>
#include <memory>  // unique_ptr
#include <string>
#include <vector>

struct streamComposite {
  enum { noHexFormat = false, HexFormat = true };
  std::unique_ptr<std::byte[]> payload;
  std::unique_ptr<rdb::payLoadAccessor<std::byte>> accessor;

  std::any get(int position);
  void set(int position, std::any value);

  streamComposite(rdb::Descriptor descriptor);
};

struct streamInstance {
  std::unique_ptr<rdb::DataStorageAccessor<std::byte>> storage;

  std::unique_ptr<streamComposite> external;
  std::unique_ptr<streamComposite> internal;

  streamInstance(const std::string file,              //
                 const rdb::Descriptor descInternal,  //
                 const rdb::Descriptor descExternal);
};

struct dataModel : public std::map<std::string, streamInstance> {};
