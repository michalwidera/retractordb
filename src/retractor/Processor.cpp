#include "Processor.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <ranges>

// This define is required to remove deprecation of boost/bind.hpp
// some boost libraries still didn't remove dependency to boost bin
// remove this is boost will clean up on own side.
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <spdlog/spdlog.h>

#include <boost/crc.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/stacktrace.hpp>

#include "QStruct.h"
#include "SOperations.h"
#include "convertTypes.h"
#include "inputFileInstance.h"

extern "C" qTree coreInstance;

Processor *pProc = nullptr;

class dataAgregator {
 public:
  /** Length of data streams processed by processor */
  int len;
  std::vector<number> row;
  std::vector<number> raw;
  dataAgregator() {}
  dataAgregator(int len) : len(len) {
    SPDLOG_DEBUG("Create dataAgregator {}", len);
    // std::fill_n(row.begin(), len, boost::rational<int>(0));
    assert(row.size() == 0);
    for (auto i = 0; i < len; i++) row.emplace_back(boost::rational<int>(0));
  }
};

std::map<std::string, dataAgregator> gDataMap;

/** Variable that contains sources of data */
std::map<std::string, inputDF> gFileMap;

/** This function will give info how long is stream argument if argument will be
 * * instead of argument list */
int getRowSize(const query &q) { return gDataMap[q.id].row.size(); }

number getValueProc(std::string streamName, int timeOffset, int schemaOffset, bool reverse = false) {
  number retval;
  const query &q(getQuery(streamName));
  assert(timeOffset >= 0);
  const int sizeOfRollup = getRowSize(q);
  if (sizeOfRollup == 0) {
    SPDLOG_ERROR("schema size of {} is {} (uninitialized?)", streamName, sizeOfRollup);
    return retval;
  }
  if (schemaOffset >= sizeOfRollup) {
    timeOffset += schemaOffset / sizeOfRollup;
    schemaOffset %= sizeOfRollup;
  }
  if (timeOffset == 0)
    retval = gDataMap[streamName].row[schemaOffset];
  else {
    /*
    dbStream &archive = *(storage[streamName]);
    retval = archive.readData(timeOffset - 1, schemaOffset, reverse);
    */
    retval = boost::rational<int>(123, 1);
    // if (streamName == "source") {  // BUG LOGGING }{
    //  std::cerr << "2. (" << timeOffset << "," << schemaOffset << ") -> "
    //            << retval << std::endl;
    //}
  }
  return retval;
}

/** This function try to roll up argument and reads data from schema
 *  This is not finished - need to be fixed */
number getValueOfRollup(const query &q, int offset) {
  token arg[3];
  const int progSize = q.lProgram.size();
  assert(progSize < 4);
  int i = 0;
  for (auto tk : q.lProgram) arg[i++] = tk;
  const command_id cmd = arg[progSize - 1].getCommandID();
  switch (cmd) {
    case PUSH_STREAM:
      return getValueProc(arg[0].getStr_(), 0, offset);
    case STREAM_TIMEMOVE:
      /* signalRow>1 : PUSH_STREAM(signalRow), STREAM_TIMEMOVE(1) */
      return getValueProc(arg[0].getStr_(), rational_cast<int>(arg[1].getRI()), offset);
    case STREAM_DEHASH_MOD:
    case STREAM_DEHASH_DIV:
      /* signalRow&0.5 : PUSH_STREAM(signalRow), PUSH_VAL(1_2),
       * PUSH_DEHASH_DIV(0)
       */
      // TODO: This is copy&paste&fix - need to refactor
      {
        assert(q.lProgram.size() == 3);
        auto streamNameArg = arg[0].getStr_();
        assert(streamNameArg != "");
        auto rationalArgument = arg[1].getRI();
        assert(rationalArgument > 0);
        // q.id - name of output stream
        // size[q.id] - count of record in output stream
        // q.rInterval - delta of output stream (rational) - contains
        // deltaDivMod( core0.delta , rationalArgument ) rationalArgument -
        // (2/3) argument of operation (rational)
        int newTimeOffset = -1;  // catch on assert(false);
        if (cmd == STREAM_DEHASH_DIV) newTimeOffset = Div(q.rInterval, rationalArgument, 0);
        if (cmd == STREAM_DEHASH_MOD) newTimeOffset = Mod(rationalArgument, q.rInterval, 0);
        if (newTimeOffset < 0) assert(false);
        return getValueProc(streamNameArg, newTimeOffset, offset);
      }
    case STREAM_SUBSTRACT:
      // TODO: Check
      return getValueProc(arg[0].getStr_(), 0, offset);
    case STREAM_AVG:
    case STREAM_MIN:
    case STREAM_MAX:
      assert(offset == 1);
      return getValueProc(arg[0].getStr_(), 0, offset);
    case STREAM_SUM:
      assert(arg[0].getStr_() != "");
      {
        assert(q.lProgram.size() == 2);
        boost::rational<int> ret = 0; /* limits.h */
        for (auto f : getQuery(arg[0].getStr_()).lSchema) {
          int pos = boost::rational_cast<int>(f.getFirstFieldToken().getRI());
          auto schema = f.getFirstFieldToken().getStr_();
          auto val = getValueProc(schema, 0, pos);
          ret += boost::rational<int>(val);
        }
        return ret;
      }
    case STREAM_ADD:
      /* signalRow+source :
          PUSH_STREAM(signalRow) arg[0]
          PUSH_STREAM(source)    arg[1]
          STREAM_ADD(0)          arg[2]
      */
      {
        const auto sizeOfFirstSchema = getQuery(arg[0].getStr_()).lSchema.size();
        if (offset < sizeOfFirstSchema)
          return getValueProc(arg[0].getStr_(), 0, offset);
        else
          return getValueProc(arg[1].getStr_(), 0, offset - sizeOfFirstSchema);
      }
    case STREAM_AGSE:
      // PUSH_STREAM core -> delta_source (argument1) arg[0]
      // PUSH_VAL 2 -> window_length (argument2) arg[1]
      // STREAM_AGSE 3 -> window_step (operation) arg[2]
      // TODO: This is copy&paste&fix - need to refactor
      // This code I've wrote when when was late - 1.00 AM
      // need to put more effort to analysis expecially for
      // if (mirror) - two conditions
      {
        assert(q.lProgram.size() == 3);
        std::string nameSrc = arg[0].getStr_();
        std::string nameOut = q.id;
        bool mirror = arg[2].getRI() < 0;
        // step - means step of how long moving window will over data stream
        // step is counted in tuples/atribute
        // windowsSize - is length of moving data window
        int step = rational_cast<int>(arg[1].getRI());
        assert(step >= 0);
        int windowSize = abs(rational_cast<int>(arg[2].getRI()));
        assert(windowSize > 0);
        int schemaSizeSrc = gDataMap[nameSrc].row.size();
        int schemaSizeOut = gDataMap[nameOut].row.size();
        int streamSizeSrc = gDataMap[nameSrc].len;
        int streamSizeOut = gDataMap[nameOut].len;
        assert(nameSrc != "");
        assert(windowSize == schemaSizeOut);
        if (mirror)
          return getValueProc(nameSrc, 0, windowSize - 1 + offset);
        else {
          int d = abs((streamSizeOut)-agse(streamSizeSrc * schemaSizeSrc, step));
          return getValueProc(nameSrc, 0, windowSize - offset + d);
        }
      }
    case STREAM_HASH:
      // TODO: Check if right hash part is returned here
      {
        const auto timeSeqence = (gDataMap[q.id].len) > 1 ? gDataMap[q.id].len : 1;
        int timeOffset = 0;
        if (Hash(getQuery(arg[0].getStr_()).rInterval, getQuery(arg[1].getStr_()).rInterval, timeSeqence, timeOffset))
          return getValueProc(arg[1].getStr_(), timeOffset, offset);
        else
          return getValueProc(arg[0].getStr_(), timeOffset, offset);
      }
      assert(false);  // TODO
  }
  SPDLOG_ERROR("Unknown operator catched cmd={} progsize={}", cmd, progSize);
  assert(false);    // Unknown operator catched here
  return number(0); /* pro forma */
}

Processor::Processor() {
  // This function initialize map creating archive streams in cbuff
  for (auto q : coreInstance) {
    /*
    if (storage.find(q.id) == storage.end())
      storage[q.id] = std::shared_ptr<dbStream>(new dbStream(q.id, q.lSchema));
    else
      ;  // Stream with this name already exist in stystem
    */
    // Container initialization for file type data sources
    if (!q.filename.empty()) gFileMap[q.id] = inputDF(q.filename.c_str(), q.lSchema);
    SPDLOG_INFO("Req: gDataMap {} len:{} rp:{}", q.id, gDataMap[q.id].row.size(), q.lSchema.size());
    gDataMap[q.id] = dataAgregator(q.lSchema.size());
    SPDLOG_INFO("Bld: gDataMap {} len:{} rp:{}", q.id, gDataMap[q.id].row.size(), getRowSize(q));
  }
  pProc = this;
}

void Processor::processRows(std::set<std::string> inSet) {
  // Update of context variable
  for (auto q : coreInstance) {  // For all queries in system
    // Drop off rows that not computed now
    if (inSet.find(q.id) == inSet.end()) continue;
    // If given stream is already synchronized with context
    // there is no sens to make computed twice
    // if (gDataMap[q.id].len == gDataMap[q.id].size) continue;
    if (gDataMap[q.id].row.size() == 0) continue;
    std::vector<number> rowValues;
    if (q.isDeclaration()) {
      // If argument is declared -
      // we read source and store in context
      assert(q.filename != "");
      rowValues = gFileMap[q.id].getFileRow();
    } else {
      // execution of stream program and store data
      assert(q.lProgram.size() < 4);  // we assume that all stream programs are
      // optimized and 1v2v3 size
      // we assume that optimized streams are ready and filled
      assert(q.lProgram.size() > 0);
      auto it = q.lProgram.begin();
      token operation = q.lProgram.back();  // Operation is always last element on stack
      token argument1, argument2;           // This arguments are optionally fulfilled
      std::string streamNameArg;            // Same as argument
      int TimeOffset(0);
      assert(rowValues.empty());
      // all stream operations cover
      switch (operation.getCommandID()) {
        case PUSH_STREAM:
          // PUSH_STREAM core0
          // push stream operation does not have additional stack arguments
          streamNameArg = operation.getStr_();
          {
            auto pos = 0;
            for (auto f : getQuery(streamNameArg).lSchema) {
              // rowValues.push_back(row[streamNameArg][pos++]);
              std::string schema = f.getFirstFieldToken().getStr_();
              rowValues.push_back(getValueProc(schema, 0, pos++));
            }
          }
          break;
        case STREAM_HASH:
          // PUSH_STREAM core0
          // PUSH_STREAM core1
          // STREAM_HASH
          assert(q.lProgram.size() == 3);
          // hash operations have two additional arguments of stack
          // these argument are stream names to HASH operation
          TimeOffset = 0;
          argument1 = *(it++);
          argument2 = *(it++);
          if (Hash(getQuery(argument1.getStr_()).rInterval, getQuery(argument2.getStr_()).rInterval, gDataMap[q.id].len,
                   TimeOffset))
            streamNameArg = argument2.getStr_();
          else
            streamNameArg = argument1.getStr_();
          if (TimeOffset <= 0) {
            SPDLOG_ERROR("Timeoffset is (<=0) = {}, arg1={}, arg2={} q.id={}", TimeOffset, argument1.getStr_(),
                         argument2.getStr_(), q.id);
          }
          // TimeOffset = TimeOffset - gDataMap[streamNameArg].len;  - need to
          // investigate here.
          if (TimeOffset < 0) {
            SPDLOG_ERROR("Timeoffset is negative = {}, len = {}, gLen={}", TimeOffset, gDataMap[q.id].len,
                         gDataMap[streamNameArg].len);
          }
          assert(TimeOffset >= 0);
          // If this assert fails - you are probably trying to hash a#b when a
          // and b has different schema sizes. This should be identified on
          // compilation phase in future.
          assert(gDataMap[streamNameArg].row.size() == getQuery(argument1.getStr_()).lSchema.size());
          assert(gDataMap[streamNameArg].row.size() == getQuery(argument2.getStr_()).lSchema.size());
          rowValues = getRow(streamNameArg, TimeOffset);
          break;
        case STREAM_ADD:
          // PUSH_STREAM core0
          // PUSH_STREAM core1
          // STREAM_ADD
          assert(q.lProgram.size() == 3);
          argument1 = *(it++);
          argument2 = *(it++);
          assert(streamNameArg == "");
          assert(argument1.getStr_() != "");
          assert(argument2.getStr_() != "");
          for (auto f : q.lSchema) rowValues.push_back(computeValue(f, q));
          // same as below ... - wait for c++20
          // std::ranges::for_each(
          //    q.lSchema, [rowValues, this, q](field f) mutable
          //    { rowValues.push_back(computeValue(f, q)); });
          break;
        case STREAM_AVG:
          argument1 = *(it++);
          streamNameArg = argument1.getStr_();
          assert(streamNameArg != "");
          {
            assert(q.lProgram.size() == 2);
            number ret = 0;
            for (auto f : getQuery(streamNameArg).lSchema) {
              int pos = boost::rational_cast<int>(f.getFirstFieldToken().getRI());
              std::string schema = f.getFirstFieldToken().getStr_();
              ret = ret + getValueProc(schema, 0, pos);
            }
            ret = ret / static_cast<int>(q.lSchema.size());
            rowValues.push_back(ret);
          }
          break;
        case STREAM_MAX:
          argument1 = *(it++);
          streamNameArg = argument1.getStr_();
          assert(streamNameArg != "");
          {
            assert(q.lProgram.size() == 2);
            number ret = INT_MIN; /* limits.h */
            for (auto f : getQuery(streamNameArg).lSchema) {
              int pos = boost::rational_cast<int>(f.getFirstFieldToken().getRI());
              std::string schema = f.getFirstFieldToken().getStr_();
              number val = getValueProc(schema, 0, pos);
              if (val > ret) ret = val;
            }
            rowValues.push_back(ret);
          }
          break;
        case STREAM_MIN:
          argument1 = *(it++);
          streamNameArg = argument1.getStr_();
          assert(streamNameArg != "");
          {
            assert(q.lProgram.size() == 2);
            number ret = INT_MAX; /* limits.h */
            for (auto f : getQuery(streamNameArg).lSchema) {
              int pos = boost::rational_cast<int>(f.getFirstFieldToken().getRI());
              std::string schema = f.getFirstFieldToken().getStr_();
              number val = getValueProc(schema, 0, pos);
              if (val < ret) ret = val;
            }
            rowValues.push_back(ret);
          }
          break;
        case STREAM_SUM:
          argument1 = *(it++);
          streamNameArg = argument1.getStr_();
          assert(streamNameArg != "");
          {
            assert(q.lProgram.size() == 2);
            number ret = 0; /* limits.h */
            for (auto f : getQuery(streamNameArg).lSchema) {
              int pos = boost::rational_cast<int>(f.getFirstFieldToken().getRI());
              std::string schema = f.getFirstFieldToken().getStr_();
              number val = getValueProc(schema, 0, pos);
              ret += val;
            }
            rowValues.push_back(ret);
          }
          break;
        case STREAM_TIMEMOVE:
          // PUSH_STREAM core1
          // STREAM_TIMEMOVE 10
          assert(q.lProgram.size() == 2);
          argument1 = *(it++);
          streamNameArg = argument1.getStr_();
          assert(streamNameArg != "");
          TimeOffset = rational_cast<int>(operation.getRI());
          assert(TimeOffset >= 0);
          rowValues = getRow(streamNameArg, TimeOffset);
          break;
        case STREAM_DEHASH_DIV:
        case STREAM_DEHASH_MOD:
          // PUSH_STREAM core0
          // PUSH_STREAM 2/3
          // STREAM_DEHASH_DIV
          {
            assert(q.lProgram.size() == 3);
            number rationalArgument;
            argument1 = *(it++);
            argument2 = *(it++);
            streamNameArg = argument1.getStr_();
            assert(streamNameArg != "");
            rationalArgument = argument2.getRI();
            assert(rationalArgument > 0);
            // q.id - name of output stream
            // size[q.id] - count of record in output stream
            // q.rInterval - delta of output stream (rational) - contains
            // deltaDivMod( core0.delta , rationalArgument ) rationalArgument -
            // (2/3) argument of operation (rational)
            int position = gDataMap[q.id].len + 1;
            // size[q.id] == -1 for zero elements (therefore + 1)
            if (operation.getCommandID() == STREAM_DEHASH_DIV) TimeOffset = Div(q.rInterval, rationalArgument, position);
            if (operation.getCommandID() == STREAM_DEHASH_MOD) TimeOffset = Mod(rationalArgument, q.rInterval, position);
            if (TimeOffset < 0) assert(false);
            rowValues = getRow(streamNameArg, TimeOffset, true);
          }
          break;
        case STREAM_AGSE:
          // PUSH_STREAM core -> delta_source (argument1)
          // PUSH_VAL 2 -> window_length (argument2)
          // STREAM_AGSE 3 -> window_step (operation)
          {
            argument1 = *(it++);
            argument2 = *(it++);
            std::string nameSrc = argument1.getStr_();
            std::string nameOut = q.id;
            bool mirror = operation.getRI() < 0;
            // step - means step of how long moving window will over data stream
            // step is counted in tuples/attribute
            // windowsSize - is length of moving data window
            int step = rational_cast<int>(argument2.getRI());
            assert(step >= 0);
            int windowSize = abs(rational_cast<int>(operation.getRI()));
            assert(windowSize > 0);
            int schemaSizeSrc = gDataMap[nameSrc].row.size();
            int schemaSizeOut = gDataMap[nameOut].row.size();
            int streamSizeSrc = gDataMap[nameSrc].len;
            int streamSizeOut = gDataMap[nameOut].len;
            assert(nameSrc != "");
            assert(windowSize == schemaSizeOut);
            if (mirror) {
              for (int i = windowSize - 1; i >= 0; i--) {
                number ret = getValueProc(nameSrc, 0, i);
                rowValues.push_back(ret);
              }
            } else {
              int d = abs((streamSizeOut)-agse(streamSizeSrc * schemaSizeSrc, step));
              // "}{";
              for (int i = 0; i < windowSize; i++) {
                number ret = getValueProc(nameSrc, 0, i + d);
                rowValues.push_back(ret);
              }
            }
            assert(rowValues.size() == q.lSchema.size());
            assert(gDataMap[q.id].row.size() == rowValues.size());
          }
          break;
        case STREAM_SUBSTRACT:
          // PUSH_STREAM core
          // STREAM_SUBSTRACT 3_2
          // This need to be checked - I was tired when I wrote this
          assert(q.lProgram.size() == 2);
          argument1 = *(it++);
          streamNameArg = argument1.getStr_();
          assert(streamNameArg != "");
          if (operation.getRI() > q.rInterval) {
            // Check if parameters are in opposite order
            TimeOffset = Substract(q.rInterval, operation.getRI(), gDataMap[q.id].len);
            TimeOffset = gDataMap[q.id].len - TimeOffset;
          }
          rowValues = getRow(streamNameArg, TimeOffset);
          break;
        default:
          // Stack hits non supported operation
          assert(false);
      }
    }
    // Store to conext computed values
    assert(!rowValues.empty());
    gDataMap[q.id].row = rowValues;
    gDataMap[q.id].len++;
  }
  // Original processRows is start here
  for (auto q : coreInstance) {
    // Drop rows that not come with this to compute
    if (inSet.find(q.id) == inSet.end()) continue;
    if (q.isDeclaration()) {
      int cnt(0);
      assert(!q.filename.empty());
      gDataMap[q.id].row = gFileMap[q.id].getFileRow();
    } else {
      for (auto i = 0; i < getRowSize(q); i++) gDataMap[q.id].row[i] = getValueOfRollup(q, i);
    }
    // here should be computer values of stream tuples
    // computed value should be stored in file
    std::vector<number> rowValues;
    int cnt(0);
    for (auto &f : q.lSchema) rowValues.push_back(computeValue(f, q));
    gDataMap[q.id].row = rowValues;
    // Store computed values to cbuffer - on disk
    // storage[q.id]->store();
  }
}

std::vector<number> Processor::getRow(std::string streamName, int timeOffset, bool reverse) {
  std::vector<number> retVal;
  if (timeOffset < 0) {
    // This need more investigation - this is kind of workaround.
    // Because from different questions sometimes -1 a timeOffset could appear.
    for (auto f : getQuery(streamName).lSchema) retVal.push_back(boost::rational<int>(0));
    return retVal;
  }
  int column = 0;
  for (auto f : getQuery(streamName).lSchema) retVal.push_back(getValueProc(streamName, timeOffset, column++, reverse));
  return retVal;
}

number Processor::computeValue(field &f, query &q) {
  std::stack<number> rStack;
  number a, b;
  for (auto tk : f.lProgram) {
    switch (tk.getCommandID()) {
      case ADD:
      case SUBTRACT:
      case MULTIPLY:
      case DIVIDE:
        a = rStack.top();
        rStack.pop();
        b = rStack.top();
        rStack.pop();
    }
    switch (tk.getCommandID()) {
      case PUSH_VAL:
        rStack.push(tk.getRI());
        break;
      case ADD:
        rStack.push(b + a);
        break;
      case SUBTRACT:
        rStack.push(b - a);
        break;
      case MULTIPLY:
        rStack.push(b * a);
        break;
      case DIVIDE:
        if (a != 0)
          rStack.push(b / a);
        else {
          rStack.push(0);
          assert(false);
        }
        break;
      case NEGATE:
        a = rStack.top();
        rStack.pop();
        rStack.push(-a);
        break;
      case CALL: {
        double real = (double)a.numerator() / (double)a.denominator();
        if (tk.getStr_() == "floor") {
          a = rStack.top();
          rStack.pop();
          rStack.push(Rationalize(floor(real)));
        } else if (tk.getStr_() == "getRowValueceil") {
          a = rStack.top();
          rStack.pop();
          rStack.push(Rationalize(ceil(real)));
        } else if (tk.getStr_() == "iszero") {
          a = rStack.top();
          rStack.pop();
          if (a.numerator() == 0)
            rStack.push(Rationalize(1));
          else
            rStack.push(Rationalize(0));
        } else if (tk.getStr_() == "isnonzero") {
          a = rStack.top();
          rStack.pop();
          if (a.numerator() == 0)
            rStack.push(Rationalize(0));
          else
            rStack.push(Rationalize(1));
        } else if (tk.getStr_() == "sqrt") {
          a = rStack.top();
          rStack.pop();
          rStack.push(Rationalize(sqrt(real)));
        } else if (tk.getStr_() == "sum") {
          number data_sum(0);
          for (int i = 0; i < getRowSize(q); i++) data_sum += getValueOfRollup(q, i);
          rStack.push(data_sum);
        } else
          throw std::out_of_range("No support for this math function - write it SVP");
      } break;
      case PUSH_ID3:  // Field_name
      case PUSH_ID1:  // Schema_name.Field_name
      case PUSH_ID2:  // Schema_name[indeks]
      case PUSH_ID4:  // Schema_name[indeks,indeks]
      case PUSH_ID5:  // Schema_name[indeks][indeks]
      case PUSH_IDX:  // Schema_name[_]
        throw std::out_of_range(
            "Thrown/Processor.cpp/computeValue This field is "
            "reducted in compilation");
        break;
      case PUSH_ID: {  // Schema_name[indeks,indeks] like PUSH_ID4
        int offsetInSchema(rational_cast<int>(tk.getRI()));
        std::string argument(tk.getStr_());
        // If operation ADD exist - then position schema will be moved
        // first argument
        int offsetFromArg = 0;
        {
          const std::string streamName(q.id);
          const std::string streamArgument(argument);
          // Function will return offsets according to stack operations
          // when A#B offsets A i B are equal , when A+B then A=0, B=0+Size(A)
          query &qq(getQuery(streamName));
          if (qq.is(STREAM_ADD)) {
            auto [sArg1, sArg2, cmd]{GetArgs(qq.lProgram)};
            if (sArg1 == argument)
              offsetFromArg = 0;
            else if (sArg2 == argument)
              offsetFromArg = getQuery(sArg1).lSchema.size();
            else {
              SPDLOG_ERROR(
                  "Call to schema that not exist from:{}, argument:{}, 1st:{}, "
                  "2nd:{}",
                  streamName, streamArgument, sArg1, sArg2);
              throw std::out_of_range("Call to schema that not exist");
            }
          }
        }
        number a = getValueProc(q.id, 0, offsetInSchema + offsetFromArg);
        rStack.push(a);
      } break;
      default:
        throw std::out_of_range("Thrown/Processor.cpp/getCRValue Unknown token");
    }
  }  // for(auto tk : f.lProgram)
  if (rStack.size() == 0) {
    assert(q.isDeclaration());
    number retVal = gDataMap[q.id].row[q.getFieldIndex(f)];
    return retVal;
  } else if (rStack.size() == 1)
    return rStack.top();
  else
    throw std::out_of_range("Thrown/Processor.cpp/getCRValue too much token left on stack");
  return 0; /* pro forma */
}

int getStreamCount(const std::string &query_name) { return gDataMap[query_name].len; }

long streamStoredSize(std::string &filename) { return 0; }
