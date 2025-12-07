
// Generated from DESC.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  DESCLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, BYTE_T = 6, STRING_T = 7, 
    UNSIGNED_T = 8, INTEGER_T = 9, FLOAT_T = 10, DOUBLE_T = 11, RATIONAL_T = 12, 
    INTPAIR_T = 13, IDXPAIR_T = 14, TYPE_T = 15, REF_T = 16, RETENTION_T = 17, 
    RETMEMORY_T = 18, DOT = 19, MINUS = 20, ID = 21, STRING = 22, DECIMAL = 23, 
    FILENAME = 24, SPACE = 25, COMMENT = 26, LINE_COMMENT1 = 27, LINE_COMMENT2 = 28
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

