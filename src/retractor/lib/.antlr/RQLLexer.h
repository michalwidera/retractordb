
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
    DO = 38, IN = 39, TO = 40, MIN = 41, MAX = 42, AVG = 43, SUMC = 44, 
    STRING_SUBSTRAT = 45, ID = 46, STRING = 47, FLOAT = 48, DECIMAL = 49, 
    REAL = 50, EQUAL = 51, GREATER = 52, LESS = 53, EXCLAMATION = 54, DOUBLE_BAR = 55, 
    DOT = 56, UNDERLINE = 57, AT = 58, SHARP = 59, AND = 60, MOD = 61, DOLLAR = 62, 
    COMMA = 63, SEMI = 64, COLON = 65, DOUBLE_COLON = 66, STAR = 67, DIVIDE = 68, 
    PLUS = 69, MINUS = 70, BIT_NOT = 71, BIT_OR = 72, BIT_XOR = 73, AND_C = 74, 
    OR_C = 75, SPACE = 76, COMMENT = 77, LINE_COMMENT1 = 78, LINE_COMMENT2 = 79
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

