#include "Processor.h"

#include "QStruct.h"
#include "SOperations.h"

// This define is required to remove deprecation of boost/bind.hpp
// some boost libraries still didn't remove dependency to boost bin
// remove this is boost will clean up on own side.
#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <algorithm>
#include <boost/crc.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/stacktrace.hpp>
#include <cstdint>
#include <iostream>
#include <ranges>

extern "C" qTree coreInstance;

Processor *pProc = nullptr;

/** This function will give info how long is stream argument if argument will be
 * * instead of argument list */
int getSizeOfRollup(const query &q) {
  token arg[3];
  auto progSize = q.lProgram.size();
  assert(progSize < 4);
  auto i = 0;
  for (auto tk : q.lProgram) arg[i++] = tk;
  if (progSize == 0) return q.lSchema.size();
  if (progSize == 1) return getQuery(arg[0].getStr()).lSchema.size();
  if (progSize == 2) return getQuery(arg[0].getStr()).lSchema.size();
  const command_id cmd = arg[progSize - 1].getCommandID();
  if (progSize == 3) {
    switch (cmd) {
      case STREAM_HASH:
      case STREAM_DEHASH_MOD:
      case STREAM_DEHASH_DIV:
      case STREAM_SUBSTRACT:
      case STREAM_TIMEMOVE:
        return getQuery(arg[0].getStr()).lSchema.size();
      case STREAM_AGSE:
        return abs(rational_cast<int>(arg[2].get()));
      case STREAM_ADD:
        return getQuery(arg[0].getStr()).lSchema.size() +
               getQuery(arg[1].getStr()).lSchema.size();
      case STREAM_AVG:
      case STREAM_MIN:
      case STREAM_MAX:
      case STREAM_SUM:
        return 1;
      default:
        assert(false);
    }
  }
  assert(false);
  return 0;  // pro forma
}

number Processor::getValueOfRollup(const query &q, int offset, int timeOffset) {
  token arg[3];
  const int progSize = q.lProgram.size();
  assert(progSize < 4);
  int i = 0;
  for (auto tk : q.lProgram) arg[i++] = tk;
  const command_id cmd = arg[progSize - 1].getCommandID();
  int TimeOffset(-1);  // This -1 is intentionally wrong - Hash return value
  switch (cmd) {
    case PUSH_STREAM:
      return getValueProc(arg[0].getStr(), timeOffset, offset);
    case STREAM_TIMEMOVE:
      /* signalRow>1 : PUSH_STREAM(signalRow), STREAM_TIMEMOVE(1) */
      return getValueProc(arg[0].getStr(),
                          timeOffset + rational_cast<int>(arg[1].get()),
                          offset);
    case STREAM_DEHASH_MOD:
    case STREAM_DEHASH_DIV:
      /* signalRow&0.5 : PUSH_STREAM(signalRow), PUSH_VAL(1_2),
       * PUSH_DEHASH_DIV(0)
       */
      // TODO: This is copy&paste&fix - need to refactor
      {
        assert(q.lProgram.size() == 3);
        auto streamNameArg = arg[0].getStr();
        assert(streamNameArg != "");
        auto rationalArgument = arg[1].get();
        assert(rationalArgument > 0);
        // q.id - name of output stream
        // gStreamSize[q.id] - count of record in output stream
        // q.rInterval - delta of output stream (rational) - contains
        // deltaDivMod( core0.delta , rationalArgument ) rationalArgument -
        // (2/3) argument of operation (rational)
        int newTimeOffset = -1;
        if (cmd == STREAM_DEHASH_DIV)
          newTimeOffset = Div(q.rInterval, rationalArgument, timeOffset);
        if (cmd == STREAM_DEHASH_MOD)
          newTimeOffset = Mod(rationalArgument, q.rInterval, timeOffset);
        if (newTimeOffset < 0) assert(false);
        return getValueProc(streamNameArg, newTimeOffset, offset);
      }
    case STREAM_SUBSTRACT:
      // TODO: Check
      return getValueProc(arg[0].getStr(), timeOffset, offset);
    case STREAM_AVG:
    case STREAM_MIN:
    case STREAM_MAX:
      assert(offset == 1);
      return getValueProc(arg[0].getStr(), timeOffset, offset);
    case STREAM_SUM:
      assert(arg[0].getStr() != "");
      {
        assert(q.lProgram.size() == 2);
        boost::rational<int> ret = 0; /* limits.h */
        for (auto f : getQuery(arg[0].getStr()).lSchema) {
          int pos = boost::rational_cast<int>(f.getFirstFieldToken().get());
          std::string schema = f.getFirstFieldToken().getStr();
          boost::rational<int> val =
              std::get<boost::rational<int>>(getValueProc(schema, 0, pos));
          ret += val;
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
        const auto sizeOfFirstSchema = getQuery(arg[0].getStr()).lSchema.size();
        if (offset < sizeOfFirstSchema)
          return getValueProc(arg[0].getStr(), timeOffset, offset);
        else
          return getValueProc(arg[1].getStr(), timeOffset,
                              offset - sizeOfFirstSchema);
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
        std::string nameSrc = arg[0].getStr();
        std::string nameOut = q.id;
        bool mirror = arg[2].get() < 0;
        // step - means step of how long moving window will over data stream
        // step is counted in tuples/atribute
        // windowsSize - is length of moving data window
        int step = rational_cast<int>(arg[1].get());
        assert(step >= 0);
        int windowSize = abs(rational_cast<int>(arg[2].get()));
        assert(windowSize > 0);
        boost::rational<int> deltaSrc = getQuery(nameSrc).rInterval;
        boost::rational<int> deltaOut = getQuery(nameOut).rInterval;
        int schemaSizeSrc = getQuery(nameSrc).lSchema.size();
        int schemaSizeOut = getQuery(nameOut).lSchema.size();
        int streamSizeSrc = gContextLenMap[nameSrc];
        int streamSizeOut = gContextLenMap[nameOut];
        int streamSizeSrc_ = gStreamSize[nameSrc];
        int streamSizeOut_ = gStreamSize[nameOut];
        if (streamSizeOut_ < 0) streamSizeOut_ = 0;
        if (streamSizeSrc_ < 0) streamSizeSrc_ = 0;
        assert(nameSrc != "");
        assert(windowSize == schemaSizeOut);
        if (mirror)
          return getValueProc(nameSrc, timeOffset, windowSize - 1 + offset);
        else {
          int d =
              abs((streamSizeOut)-agse(streamSizeSrc * schemaSizeSrc, step));
          return getValueProc(nameSrc, timeOffset, windowSize - offset + d);
        }
      }
    case STREAM_HASH:
      // TODO: Check if right hash part is returned here
      {
        const auto timeSeqence = (gContextLenMap[q.id] - timeOffset) > 1
                                     ? gContextLenMap[q.id] - timeOffset
                                     : 1;
        if (Hash(getQuery(arg[0].getStr()).rInterval,
                 getQuery(arg[1].getStr()).rInterval, timeSeqence, TimeOffset))
          return getValueProc(arg[1].getStr(), TimeOffset, offset);
        else
          return getValueProc(arg[0].getStr(), TimeOffset, offset);
      }
      assert(false);  // TODO
  }
  std::cerr << "cmd=" << cmd << std::endl;
  std::cerr << "progsize=" << progSize << std::endl;
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
    if (!q.filename.empty())
      gFileMap[q.id] = inputDF(q.filename.c_str(), q.lSchema);
  }
  // Initializing fill of context and lenght of data stream
  for (auto q : coreInstance) {  // For all queries in systes
    std::vector<number> rowValues;
    for (auto i = 0; i < getSizeOfRollup(q); i++)
      rowValues.push_back(boost::rational<int>(0));
    gContextValMap[q.id] = rowValues;
    gContextLenMap[q.id] = 0;
    gStreamSize[q.id] = -1;
  }
  pProc = this;
}

void Processor::processRows(std::set<std::string> inSet) {
  for (auto q : coreInstance) {
    // Drop rows that not come with this to compute
    if (inSet.find(q.id) == inSet.end()) continue;
    gStreamSize[q.id]++;

    if (q.isDeclaration()) {
      int cnt(0);
      assert(!q.filename.empty());
      gFileMap[q.id]
          .readRowFromFile(); // Fetch next row form file that have schema
      for (auto f : q.lSchema)
        gContextValMap[q.id][cnt++] = gFileMap[q.id].getCR(f);
      // same as below ... wait for C++20
      // std::ranges::for_each(
      //    q.lSchema, [ctxRowValue, this, q](field f) mutable
      //    { ctxRowValue.push_back(gFileMap[q.id].getCR(f)); });
    } else {
      for (auto i = 0; i < getSizeOfRollup(q); i++)
        gContextValMap[q.id][i] = getValueOfRollup(q, i, 0);
    }

    // here should be computer values of stream tuples
    // computed value shoud be stored in file

    int cnt(0);
    for (auto &f : q.lSchema)
    {
      boost::rational<int> value(computeValue(f, q));
      gContextValMap[q.id][cnt++] = value;
    }
    // Store computed values to cbuffer - on disk
    // storage[q.id]->store();
  }
}

std::vector<number> Processor::getRow(std::string streamName, int timeOffset,
                                      bool reverse) {
  std::vector<number> retVal;
  int column = 0;
  for (auto f : getQuery(streamName).lSchema)
    retVal.push_back(getValueProc(streamName, timeOffset, column++, reverse));
  return retVal;
}

int Processor::getArgumentOffset(const std::string &streamName,
                                 const std::string &streamArgument) {
  int retVal = 0;
  query &q(getQuery(streamName));
  for (auto t : q.lProgram) {
    if (t.getCommandID() == STREAM_ADD) {
      auto it = q.lProgram.begin();
      token A = *it;
      it++;
      token B = *it;
      if (A.getStr() == streamArgument)
        retVal = 0;
      else if (B.getStr() == streamArgument)
        retVal = getQuery(A.getStr()).lSchema.size();
      else {
        std::cerr << "currnet stream:" << streamName << std::endl;
        std::cerr << "argument:" << streamArgument << std::endl;
        std::cerr << "1st: " << A.getStr() << std::endl;
        std::cerr << "2nd: " << B.getStr() << std::endl;
        std::cerr << "@:" << boost::stacktrace::stacktrace() << std::endl;
        throw std::out_of_range("Call to schema that not exist");
      }
    }
  }
  return retVal;
}

number Processor::getValueProc(std::string streamName, int timeOffset,
                               int schemaOffset, bool reverse) {
  number retval;
  query &q(getQuery(streamName));
  assert(timeOffset >= 0);
  if (schemaOffset >= getSizeOfRollup(q)) {
    timeOffset += schemaOffset / getSizeOfRollup(q);
    schemaOffset %= q.lSchema.size();
  }
  if ((timeOffset == 0) &&
      (gContextLenMap[streamName] > gStreamSize[streamName]))
    retval = gContextValMap[streamName][schemaOffset];
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

void Processor::updateContext(std::set<std::string> inSet) {
  // Update of context variable
  for (auto q : coreInstance) {  // For all queries in system
    // Drop off rows that not computed now
    if (inSet.find(q.id) == inSet.end()) continue;
    // If given stream is aledy synchronized with context
    // there is no sens to make computed twice
    if (gContextLenMap[q.id] == gStreamSize[q.id]) continue;
    std::vector<number> rowValues;
    if (q.isDeclaration()) {
      // If argument is declared -
      // we read source and store in context
      assert(q.filename != "");
      for (auto f : q.lSchema) rowValues.push_back(gFileMap[q.id].getCR(f));
    } else {
      // execution of stream program and store data
      assert(q.lProgram.size() < 4);  // we assume that all stream programs are
      // optimized and 1v2v3 size
      assert(q.lProgram.size() >
             0);  // we assume that optimized streams are ready and filled
      std::list<token>::iterator it = q.lProgram.begin();
      token operation =
          q.lProgram.back();       // Operation is always last element on stack
      token argument1, argument2;  // This arugments are optionally fullfiled
      std::string streamNameArg;   // Same as argument
      int TimeOffset(-1);  // This -1 is intentionally wrong to catch errors
      assert(rowValues.empty());
      // all stream operations cover
      switch (operation.getCommandID()) {
        case PUSH_STREAM:
          // PUSH_STREAM core0
          // push stream operation does not have additonal stack arguments
          streamNameArg = operation.getStr();
          {
            auto pos = 0;
            for (auto f : getQuery(streamNameArg).lSchema) {
              // rowValues.push_back(gContextValMap[streamNameArg][pos++]);
              std::string schema = f.getFirstFieldToken().getStr();
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
          argument1 = *(it++);
          argument2 = *(it++);
          if (Hash(getQuery(argument1.getStr()).rInterval,
                   getQuery(argument2.getStr()).rInterval, gContextLenMap[q.id],
                   TimeOffset))
            streamNameArg = argument2.getStr();
          else
            streamNameArg = argument1.getStr();
          TimeOffset = TimeOffset - gStreamSize[streamNameArg];
          assert(TimeOffset >= 0);
          // If this assert fails - you are probably trying to hash a#b when a
          // and b has different schema sizes. This should be identified on
          // compilation phase in future.
          assert(gContextValMap[streamNameArg].size() ==
                 getQuery(argument1.getStr()).lSchema.size());
          assert(gContextValMap[streamNameArg].size() ==
                 getQuery(argument2.getStr()).lSchema.size());
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
          assert(argument1.getStr() != "");
          assert(argument2.getStr() != "");
          for (auto f : q.lSchema) rowValues.push_back(computeValue(f, q));
          // same as below ... - wait for c++20
          // std::ranges::for_each(
          //    q.lSchema, [rowValues, this, q](field f) mutable
          //    { rowValues.push_back(computeValue(f, q)); });
          break;
        case STREAM_AVG:
          argument1 = *(it++);
          streamNameArg = argument1.getStr();
          assert(streamNameArg != "");
          {
            assert(q.lProgram.size() == 2);
            boost::rational<int> ret = 0;
            for (auto f : getQuery(streamNameArg).lSchema) {
              int pos = boost::rational_cast<int>(f.getFirstFieldToken().get());
              std::string schema = f.getFirstFieldToken().getStr();
              ret = ret + std::get<boost::rational<int>>(
                              getValueProc(schema, 0, pos));
            }
            ret = ret / static_cast<int>(q.lSchema.size());
            rowValues.push_back(ret);
          }
          break;
        case STREAM_MAX:
          argument1 = *(it++);
          streamNameArg = argument1.getStr();
          assert(streamNameArg != "");
          {
            assert(q.lProgram.size() == 2);
            boost::rational<int> ret = INT_MIN; /* limits.h */
            for (auto f : getQuery(streamNameArg).lSchema) {
              int pos = boost::rational_cast<int>(f.getFirstFieldToken().get());
              std::string schema = f.getFirstFieldToken().getStr();
              boost::rational<int> val =
                  std::get<boost::rational<int>>(getValueProc(schema, 0, pos));
              if (val > ret) ret = val;
            }
            rowValues.push_back(ret);
          }
          break;
        case STREAM_MIN:
          argument1 = *(it++);
          streamNameArg = argument1.getStr();
          assert(streamNameArg != "");
          {
            assert(q.lProgram.size() == 2);
            boost::rational<int> ret = INT_MAX; /* limits.h */
            for (auto f : getQuery(streamNameArg).lSchema) {
              int pos = boost::rational_cast<int>(f.getFirstFieldToken().get());
              std::string schema = f.getFirstFieldToken().getStr();
              boost::rational<int> val =
                  std::get<boost::rational<int>>(getValueProc(schema, 0, pos));
              if (val < ret) ret = val;
            }
            rowValues.push_back(ret);
          }
          break;
        case STREAM_SUM:
          argument1 = *(it++);
          streamNameArg = argument1.getStr();
          assert(streamNameArg != "");
          {
            assert(q.lProgram.size() == 2);
            boost::rational<int> ret = 0; /* limits.h */
            for (auto f : getQuery(streamNameArg).lSchema) {
              int pos = boost::rational_cast<int>(f.getFirstFieldToken().get());
              std::string schema = f.getFirstFieldToken().getStr();
              boost::rational<int> val =
                  std::get<boost::rational<int>>(getValueProc(schema, 0, pos));
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
          streamNameArg = argument1.getStr();
          assert(streamNameArg != "");
          TimeOffset = rational_cast<int>(operation.get());
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
            boost::rational<int> rationalArgument;
            argument1 = *(it++);
            argument2 = *(it++);
            streamNameArg = argument1.getStr();
            assert(streamNameArg != "");
            rationalArgument = argument2.get();
            assert(rationalArgument > 0);
            // q.id - name of output stream
            // gStreamSize[q.id] - count of record in output stream
            // q.rInterval - delta of output stream (rational) - contains
            // deltaDivMod( core0.delta , rationalArgument ) rationalArgument -
            // (2/3) argument of operation (rational)
            int position = gStreamSize[q.id] + 1;
            // gStreamSize[q.id] == -1 for zero elements (therefore + 1)
            if (operation.getCommandID() == STREAM_DEHASH_DIV)
              TimeOffset = Div(q.rInterval, rationalArgument, position);
            if (operation.getCommandID() == STREAM_DEHASH_MOD)
              TimeOffset = Mod(rationalArgument, q.rInterval, position);
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
            std::string nameSrc = argument1.getStr();
            std::string nameOut = q.id;
            bool mirror = operation.get() < 0;
            // step - means step of how long moving window will over data stream
            // step is counted in tuples/atribute
            // windowsSize - is length of moving data window
            int step = rational_cast<int>(argument2.get());
            assert(step >= 0);
            int windowSize = abs(rational_cast<int>(operation.get()));
            assert(windowSize > 0);
            boost::rational<int> deltaSrc = getQuery(nameSrc).rInterval;
            boost::rational<int> deltaOut = getQuery(nameOut).rInterval;
            int schemaSizeSrc = getQuery(nameSrc).lSchema.size();
            int schemaSizeOut = getQuery(nameOut).lSchema.size();
            int streamSizeSrc = gContextLenMap[nameSrc];
            int streamSizeOut = gContextLenMap[nameOut];
            int streamSizeSrc_ = gStreamSize[nameSrc];
            int streamSizeOut_ = gStreamSize[nameOut];
            if (streamSizeOut_ < 0) streamSizeOut_ = 0;
            if (streamSizeSrc_ < 0) streamSizeSrc_ = 0;
            assert(nameSrc != "");
            assert(windowSize == schemaSizeOut);
            if (mirror) {
              for (int i = windowSize - 1; i >= 0; i--) {
                number ret = getValueProc(nameSrc, 0, i);
                rowValues.push_back(ret);
              }
            } else {
              int d = abs(
                  (streamSizeOut)-agse(streamSizeSrc * schemaSizeSrc, step));
              std::cerr << "}{";
              for (int i = 0; i < windowSize; i++) {
                number ret = getValueProc(nameSrc, 0, i + d);
                // std::cerr << "1. " << i + d << " -> " << ret << std::endl;
                rowValues.push_back(ret);
              }
              std::cerr << std::endl;
            }
            assert(rowValues.size() == q.lSchema.size());
            assert(gContextValMap[q.id].size() == rowValues.size());
          }
          break;
        case STREAM_SUBSTRACT:
          // PUSH_STREAM core
          // STREAM_SUBSTRACT 3_2
          // This need to be checked - I was tired when I wrote this
          assert(q.lProgram.size() == 2);
          argument1 = *(it++);
          streamNameArg = argument1.getStr();
          assert(streamNameArg != "");
          if (operation.get() > q.rInterval) {
            // Check if parameters are in oposite order
            TimeOffset =
                Substract(q.rInterval, operation.get(), gStreamSize[q.id]);
            TimeOffset = gContextLenMap[q.id] - TimeOffset;
            rowValues = getRow(streamNameArg, TimeOffset);
          } else
            rowValues = gContextValMap[streamNameArg];
          break;
        default:
          // Stack hits non supported opertation
          assert(false);
      }
    }
    // Store to conext computed values
    assert(!rowValues.empty());
    gContextValMap[q.id] = rowValues;
    gContextLenMap[q.id]++;
  }
}

boost::rational<int> Processor::computeValue(field &f, query &q) {
  std::stack<boost::rational<int>> rStack;
  boost::rational<int> a, b;
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
        rStack.push(tk.get());
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
        if (tk.getStr() == "floor") {
          a = rStack.top();
          rStack.pop();
          rStack.push(boost::rational<int>(Rationalize(floor(real))));
        } else if (tk.getStr() == "getRowValueceil") {
          a = rStack.top();
          rStack.pop();
          rStack.push(boost::rational<int>(Rationalize(ceil(real))));
        } else if (tk.getStr() == "iszero") {
          a = rStack.top();
          rStack.pop();
          if (a.numerator() == 0)
            rStack.push(boost::rational<int>(Rationalize(1)));
          else
            rStack.push(boost::rational<int>(Rationalize(0)));
        } else if (tk.getStr() == "isnonzero") {
          a = rStack.top();
          rStack.pop();
          if (a.numerator() == 0)
            rStack.push(boost::rational<int>(Rationalize(0)));
          else
            rStack.push(boost::rational<int>(Rationalize(1)));
        } else if (tk.getStr() == "sqrt") {
          a = rStack.top();
          rStack.pop();
          rStack.push(boost::rational<int>(Rationalize(sqrt(real))));
        } else if (tk.getStr() == "sum") {
          int data_sum(0);
          for (int i = 0; i < getSizeOfRollup(q); i++) {
            boost::rational<int> val =
                std::get<boost::rational<int>>(getValueOfRollup(q, i, 0));
            data_sum += rational_cast<int>(val);
          }
          boost::rational<int> val(data_sum);
          rStack.push(val);
        } else if (tk.getStr() == "count") {
          assert(rStack.size() < 4);
          assert(rStack.size() > 0);
          /**
           * Overhere you should see 1,2 or 3 arguments
           * (x) means count X in schema
           * (x,y) means count from X to Y in schema
           * (x,y,t) means count X to Y in schemat and t probes back
           */
          std::vector<int> args;
          while (!rStack.empty()) {
            args.push_back(rational_cast<int>(rStack.top()));
            rStack.pop();
          }
          int from, to, len = 1;
          if (args.size() == 1) from = to = args[0];
          if (args.size() == 2) {
            from = args[1];
            to = args[0];
          }
          if (args.size() == 3) {
            from = args[2];
            to = args[1];
            len = args[0];
          }
          if (to < from) {
            int tempval = from;
            to = from;
            from = tempval;
          }
          boost::rational<int> ret = 0; /* limits.h */
          for (int j = 0; j < len; j++)
            for (int i = 0; i < getSizeOfRollup(q); i++) {
              boost::rational<int> val =
                  std::get<boost::rational<int>>(getValueOfRollup(q, i, j));
              if (val >= from && val <= to) ret++;
            }
          rStack.push(ret);
        } else
          throw std::out_of_range(
              "No support for this math function - write it SVP");
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
        int offsetInSchema(rational_cast<int>(tk.get()));
        std::string argument(tk.getStr());
        // If operation ADD exist - then position schema will be moved
        // first argument
        int offsetFromArg = getArgumentOffset(q.id, argument);
        number a = getValueProc(q.id, 0, offsetInSchema + offsetFromArg);
        rStack.push(std::get<boost::rational<int>>(a));
      } break;
      default:
        throw std::out_of_range(
            "Thrown/Processor.cpp/getCRValue Unknown token");
    }
  }  // for(auto tk : f.lProgram)
  if (rStack.size() == 0) {
    assert(q.isDeclaration());
    int fieldPosition = q.getFieldIndex(f);
    assert(fieldPosition != -1);
    number retVal = gContextValMap[q.id][fieldPosition];
    return std::get<boost::rational<int>>(retVal);
  } else if (rStack.size() == 1)
    return rStack.top();
  else
    throw std::out_of_range(
        "Thrown/Processor.cpp/getCRValue too much token left on stack");
  return boost::rational<int>(0); /* pro forma */
}

int Processor::getStreamCount(const std::string query_name) {
  return gStreamSize[query_name];
}

long streamStoredSize(std::string filename) { return 0; }
