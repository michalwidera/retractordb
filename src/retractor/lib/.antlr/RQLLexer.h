
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
    FILE = 32, STORAGE = 33, SUBSTRAT = 34, RULE = 35, ON = 36, WHEN = 37, 
    DUMP = 38, SYSTEM = 39, DO = 40, TO = 41, MIN = 42, MAX = 43, AVG = 44, 
    SUMC = 45, STRING_SUBSTRAT = 46, ID = 47, STRING = 48, FLOAT = 49, DECIMAL = 50, 
    REAL = 51, EQUAL = 52, GREATER = 53, LESS = 54, EXCLAMATION = 55, DOUBLE_BAR = 56, 
    DOT = 57, UNDERLINE = 58, AT = 59, SHARP = 60, AND = 61, MOD = 62, DOLLAR = 63, 
    COMMA = 64, SEMI = 65, COLON = 66, DOUBLE_COLON = 67, STAR = 68, DIVIDE = 69, 
    PLUS = 70, MINUS = 71, BIT_NOT = 72, BIT_OR = 73, BIT_XOR = 74, AND_C = 75, 
    OR_C = 76, SPACE = 77, COMMENT = 78, LINE_COMMENT1 = 79, LINE_COMMENT2 = 80
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

