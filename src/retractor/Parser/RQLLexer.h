
// Generated from RQL.g4 by ANTLR 4.13.0

#pragma once


#include "antlr4-runtime.h"




class  RQLLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, T__18 = 19, T__19 = 20, 
    STRING_T = 21, BYTEARRAY_T = 22, INTARRAY_T = 23, BYTE_T = 24, UNSIGNED_T = 25, 
    INTEGER_T = 26, FLOAT_T = 27, DOUBLE_T = 28, SELECT = 29, STREAM = 30, 
    FROM = 31, DECLARE = 32, FILE = 33, STORAGE = 34, MIN = 35, MAX = 36, 
    AVG = 37, SUMC = 38, ID = 39, STRING = 40, FLOAT = 41, DECIMAL = 42, 
    REAL = 43, EQUAL = 44, GREATER = 45, LESS = 46, EXCLAMATION = 47, DOUBLE_BAR = 48, 
    DOT = 49, UNDERLINE = 50, AT = 51, SHARP = 52, AND = 53, MOD = 54, DOLLAR = 55, 
    COMMA = 56, SEMI = 57, COLON = 58, DOUBLE_COLON = 59, STAR = 60, DIVIDE = 61, 
    PLUS = 62, MINUS = 63, BIT_NOT = 64, BIT_OR = 65, BIT_XOR = 66, SPACE = 67, 
    COMMENT = 68, LINE_COMMENT = 69
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

