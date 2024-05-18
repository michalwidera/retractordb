
// Generated from DESC.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  DESCLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, BYTE_T = 3, STRING_T = 4, UNSIGNED_T = 5, INTEGER_T = 6, 
    FLOAT_T = 7, DOUBLE_T = 8, RATIONAL_T = 9, INTPAIR_T = 10, IDXPAIR_T = 11, 
    TYPE_T = 12, REF_T = 13, ID = 14, STRING = 15, REF_TYPE_ARG = 16
  };

  explicit DESCLexer(antlr4::CharStream *input);

  ~DESCLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

