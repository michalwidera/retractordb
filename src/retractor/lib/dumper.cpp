#include "dumper.h"

#include <boost/lexical_cast.hpp>  // for lexical_cast
#include <boost/regex.hpp>         // IWYU pragma: keep
#include <boost/system/error_code.hpp>
#include <fstream>
#include <iostream>  // for operator<<

#include "config.h"  // Add an automatically generated configuration file

using namespace boost;

// Object coreInstance in QStruct.cpp

//
// 1. dumper.exe out_p.qry --dot >out.dot
// 2. dot -Tjpg out.dot -o file.jpg
// 3. start file.jgp
//
void dumper::graphiz(std::ostream &xout, bool bShowFileds, bool bShowStreamProgs, bool bShowTags) {
  //
  // dot call commandline: dot -Tjpg filewithgraph.txt -o file.jpg
  //
  xout << "digraph structs {" << std::endl;
  xout << " node\t[shape=record];" << std::endl;
  xout << "";
  std::set<std::string> planStreamRelationsSet;
  for (auto q : coreInstance) {
    if (q.id == ":STORAGE") continue;
    //
    // Stream presentation
    //
    xout << " ";
    xout << q.id << "\t";
    xout << "[shape=record,";
    if (q.isDeclaration())
      xout << "style=filled,fillcolor=Skyblue,color=Black,";
    else if (q.isGenerated())
      xout << "style=filled,fillcolor=Sienna,color=Black,";
    else
      xout << "style=\"filled\",fillcolor=Gray,color=Black,";
    xout << "label=\"";
    xout << q.id;
    xout << "\\ninterval=" << q.rInterval;
    if (q.isDeclaration()) xout << "\\nDeclaration";
    if (q.isGenerated()) xout << "\\nAutogen";
    // end stream specific
    //
    // fields in stream
    //
    if (bShowFileds) {
      xout << "|";
      xout << "{";
      int dTag(0);
      bool isFirst(true);
      for (auto f : q.lSchema) {
        if (isFirst)
          isFirst = false;
        else
          xout << "|";
        if (bShowTags) xout << "<tag" << dTag++ << ">";
        //
        // Patch on gramma problem -
        // dot program is using { as important sign - we need to convert { to <
        //
        std::string name(f.field_.rname);
        std::replace(name.begin(), name.end(), '{', '/');
        std::replace(name.begin(), name.end(), '}', '/');
        xout << name;
        std::cout << "(" << GetStringdescFld(f.field_.rtype) << ")";
      }
      xout << "}";
    }  // if ( bShowFileds ) - end of fields in stream
    //
    // stream specific
    //
    xout << "\"]";
    xout << std::endl;
    if (!q.isDeclaration())
      if (bShowStreamProgs) {
        xout << " prg_";
        xout << q.id << "\t";
        xout << "[shape=box,style=filled,";
        if (q.isReductionRequired()) {
          xout << "fillcolor=Green,color=Black,";
          xout << "label=\"";
        } else if (q.isGenerated()) {
          xout << "fillcolor=Yellow,color=Black,";
          xout << "label=\"";
        } else {
          // These are stream programs
          xout << "fillcolor=Orange,color=Black,";
          xout << "label=\"";
        }
        for (token t : q.lProgram) {
          xout << t.getStrCommandID() << " ";
          if (t.getStrCommandID() != "STREAM_HASH"           //
              && t.getStrCommandID() != "STREAM_ADD"         //
              && t.getStrCommandID() != "STREAM_DEHASH_DIV"  //
              && t.getStrCommandID() != "STREAM_DEHASH_MOD")
            xout << t.getStr_();
          xout << "\\n";
        }
        xout << "\"]";
        xout << std::endl;
        //
        // Relation stream plan to stream
        //
        std::string relation(q.id + " -> " + "prg_" + q.id);
        planStreamRelationsSet.insert(relation);
      }
  }
  for (auto s : planStreamRelationsSet) xout << s << std::endl;
  //
  // Due this variable we eleminate redundant relation on schema
  //
  std::set<std::string> streamRelationsSet;
  //
  // Programs that show build of data stream
  //
  if (bShowTags) {
    for (auto q : coreInstance) {
      int dTag(0);
      for (auto f : q.lSchema) {
        if (q.isDeclaration()) continue;
        xout << " ";
        xout << q.id << "_tag" << dTag << "\t";
        if (q.isDeclaration())
          xout << "[shape=record,style=filled,fillcolor=Sienna,color=Black,"
                  "label=\"";
        else
          xout << "[shape=record,label=\"";
        std::string sFieldName(f.field_.rname);
        std::replace(sFieldName.begin(), sFieldName.end(), '{', '/');
        std::replace(sFieldName.begin(), sFieldName.end(), '}', '/');
        xout << sFieldName;
        xout << "|";
        xout << "{";
        bool isFirst(true);
        if (q.isDeclaration())
          xout << "Declaration";
        else
          for (auto t : f.lProgram) {
            if (isFirst)
              isFirst = false;
            else
              xout << "|";
            std::string sTokenName(t.getStrCommandID());
            std::replace(sTokenName.begin(), sTokenName.end(), '{', '/');
            std::replace(sTokenName.begin(), sTokenName.end(), '}', '/');
            xout << t;
          }
        xout << "}";
        xout << "\"";
        xout << "]";
        xout << std::endl;
        // Relation1
        std::string relation(q.id + ":" + "tag" + lexical_cast<std::string>(dTag) + " -> " + q.id + "_tag" +
                             lexical_cast<std::string>(dTag) + " [style=dotted]");
        streamRelationsSet.insert(relation);
        ++dTag;
      }
    }
  }
  //
  // Stream realtions
  //
  for (auto q : coreInstance) {
    for (auto t : q.lProgram) {
      if (t.getStrCommandID() == "PUSH_STREAM") {
        if (q.isDeclaration()) continue;
        std::string relation(q.id + " -> " + t.getStr_());
        if (bShowStreamProgs) relation = "prg_" + relation;
        streamRelationsSet.insert(relation);
      }
    }
  }
  for (auto s : streamRelationsSet) xout << s << std::endl;
  xout << "}" << std::endl;
}

void dumper::qFieldsProgram() {
  std::cout << std::endl;
  std::cout << "fcnt_ref\tid_ref\ttoken\tvalue" << std::endl;
  for (auto q : coreInstance) {
    for (auto f : q.lSchema) {
      for (auto t : f.lProgram) {
        std::cout << q.id << "\t";
        std::cout << f.field_.rname << "\t";

        if (t.getStrCommandID() == "PUSH_ID")
          std::cout << t;
        else
          std::cout << t.getStrCommandID();

        std::cout << std::endl;
      }
    }
  }
}

void dumper::qFields() {
  std::cout << std::endl;
  std::cout << "fcnt\tid_ref\tfName" << std::endl;
  for (auto q : coreInstance) {
    int loccnt(0);
    for (auto f : q.lSchema) {
      std::cout << ++loccnt << "\t";
      std::cout << q.id << "\t";
      std::cout << f.field_.rname << "\t";
      std::cout << std::endl;
    }
  }
}

void dumper::qPrograms() {
  std::cout << std::endl;
  std::cout << "qcnt\tid_ref\ttoken\tvalue" << std::endl;
  for (auto q : coreInstance) {
    int loccnt(0);
    for (auto t : q.lProgram) {
      std::cout << ++loccnt << "\t";
      std::cout << q.id << "\t";
      std::cout << t.getStrCommandID() << "\t";
      std::cout << t.getStr_();
      std::cout << std::endl;
    }
  }
}

void dumper::qSet() {
  std::cout << std::endl;
  std::cout << "id\tlen prg\tlen sch\tinterval\tfilename" << std::endl;
  for (auto q : coreInstance) {
    std::cout << q.id << "\t";
    std::cout << (int)q.lProgram.size() << "\t";
    std::cout << (int)q.lSchema.size() << "\t";
    std::cout << q.rInterval << "\t";
    std::cout << q.filename << "\t";
    std::cout << std::endl;
  }
}

void dumper::rawTextFile() {
  for (auto q : coreInstance) {
    std::cout << q.id << "(" << q.rInterval << ")";
    if (!q.filename.empty()) std::cout << "\t" << q.filename;
    std::cout << std::endl;
    for (auto t : q.lProgram)
      if (t.getStrCommandID() == "PUSH_ID" ||          //
          t.getStrCommandID() == "PUSH_STREAM" ||      //
          t.getStrCommandID() == "PUSH_VAL" ||         //
          t.getStrCommandID() == "STREAM_SUBTRACT" ||  //
          t.getStrCommandID() == "STREAM_AGSE" ||      //
          t.getStrCommandID() == "STREAM_TIMEMOVE")
        std::cout << "\t:- " << t << std::endl;
      else
        std::cout << "\t:- " << t.getStrCommandID() << std::endl;
    for (auto f : q.lSchema) {
      std::cout << "\t";
      std::cout << f.field_.rname << ": " << GetStringdescFld(f.field_.rtype);
      std::cout << std::endl;
      for (auto tf : f.lProgram)
        if (tf.getStrCommandID() == "PUSH_ID") {
          std::cout << "\t\t" << tf << std::endl;
        } else if ((tf.getStrCommandID() == "CALL") || (tf.getStrCommandID() == "PUSH_VAL")) {
          std::cout << "\t\t" << tf << std::endl;
        } else
          std::cout << "\t\t" << tf.getStrCommandID() << std::endl;
    }
  }
}

int dumper::run(boost::program_options::variables_map &vm) {
  try {
    if (vm.count("tags") != 0 && vm.count("fields") == 0) {
      std::cerr << "Conflicting parameters." << std::endl;
      std::cerr << "Tags are referencing fields - when you set tags, leave field in dots" << std::endl;
      return system::errc::invalid_argument;
    }
    if (vm.count("dot")) {
      graphiz(std::cout, vm.count("fields") != 0, vm.count("streamprogs") != 0, vm.count("tags") != 0);
    } else if (vm.count("csv")) {
      qSet();
      qPrograms();
      qFields();
      qFieldsProgram();
    } else {
      rawTextFile();
    }
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return system::errc::interrupted;
  }
  return system::errc::success;
}
