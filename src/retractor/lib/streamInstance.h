#pragma once

#include <memory>  // unique_ptr
#include <string>

#include "QStruct.h"  // qTree
#include "dumpManager.h"
#include "rdb/descriptor.h"  // rdb::Descriptor
#include "rdb/payload.h"     // rdb::payload
#include "rdb/storageacc.h"  // rdb::storageAccessor

struct streamInstance {
  qTree &coreInstance;
  streamInstance() = delete;

  std::unique_ptr<rdb::storageAccessor> outputPayload;  // here is payload that will be stored - select clause
  std::unique_ptr<rdb::payload> inputPayload;           // payload used for computation in select
                                                        // clause - created by from clause.

  // This constructor will create data based on QStruct query
  explicit streamInstance(qTree &coreInstance, query &qry, std::string storagePathParam = "");

  rdb::payload constructAgsePayload(const int length,             //  _@(_,length)
                                    const int step,               //  _@(step,_)
                                    const std::string &instance,  //  instance@(_,_)
                                    const int storedRecordsCountDst);
  rdb::payload constructAggregate(command_id cmd, const std::string &instance);

  /*
   * This function will create OutputPayload based on all field from query
   * constructOutputPayload uses only data from inputPayload
   * inputPayload need to be filled first before this constructOutputPayload will be called.
   */
  void constructOutputPayload(const std::list<field> &fields);

  /*
   * This function will process all rules from query
   * constructRules uses data from outputPayload
   * outputPayload need to be filled first before this constructRules will be called
   */
  void constructRulesAndUpdate(query &qry);

 private:
  dumpManager dumpMgr;
};
