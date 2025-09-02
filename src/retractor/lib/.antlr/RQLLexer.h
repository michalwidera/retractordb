
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
    REAL = 51, IS_EQUAL = 52, EQUAL = 53, GREATER = 54, LESS = 55, EXCLAMATION = 56, 
    DOUBLE_BAR = 57, DOT = 58, UNDERLINE = 59, AT = 60, SHARP = 61, AND = 62, 
    MOD = 63, DOLLAR = 64, COMMA = 65, SEMI = 66, COLON = 67, DOUBLE_COLON = 68, 
    STAR = 69, DIVIDE = 70, PLUS = 71, MINUS = 72, BIT_NOT = 73, BIT_OR = 74, 
    BIT_XOR = 75, AND_C = 76, OR_C = 77, SPACE = 78, COMMENT = 79, LINE_COMMENT1 = 80, 
    LINE_COMMENT2 = 81
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

