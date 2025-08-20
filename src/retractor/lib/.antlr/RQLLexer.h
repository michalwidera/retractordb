
// Generated from RQL.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  RQLLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, T__18 = 19, T__19 = 20, 
    BYTE_T = 21, STRING_T = 22, UNSIGNED_T = 23, INTEGER_T = 24, FLOAT_T = 25, 
    DOUBLE_T = 26, SELECT = 27, STREAM = 28, FROM = 29, DECLARE = 30, RETENTION = 31, 
    FILE = 32, STORAGE = 33, SUBSTRAT = 34, MIN = 35, MAX = 36, AVG = 37, 
    SUMC = 38, STRING_SUBSTRAT = 39, ID = 40, STRING = 41, FLOAT = 42, DECIMAL = 43, 
    REAL = 44, EQUAL = 45, GREATER = 46, LESS = 47, EXCLAMATION = 48, DOUBLE_BAR = 49, 
    DOT = 50, UNDERLINE = 51, AT = 52, SHARP = 53, AND = 54, MOD = 55, DOLLAR = 56, 
    COMMA = 57, SEMI = 58, COLON = 59, DOUBLE_COLON = 60, STAR = 61, DIVIDE = 62, 
    PLUS = 63, MINUS = 64, BIT_NOT = 65, BIT_OR = 66, BIT_XOR = 67, SPACE = 68, 
    COMMENT = 69, LINE_COMMENT1 = 70, LINE_COMMENT2 = 71
  };

  explicit RQLLexer(antlr4::CharStream *input);

  ~RQLLexer() override;


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

