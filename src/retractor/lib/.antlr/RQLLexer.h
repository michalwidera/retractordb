
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
    DUMP = 38, SYSTEM = 39, DO = 40, TO = 41, AND_C = 42, OR_C = 43, NOT_C = 44, 
    MIN = 45, MAX = 46, AVG = 47, SUMC = 48, STRING_SUBSTRAT = 49, ID = 50, 
    STRING = 51, FLOAT = 52, DECIMAL = 53, REAL = 54, IS_EQ = 55, IS_NQ = 56, 
    IS_GR = 57, IS_LS = 58, IS_GE = 59, IS_LE = 60, EXCLAMATION = 61, DOUBLE_BAR = 62, 
    DOT = 63, UNDERLINE = 64, AT = 65, SHARP = 66, AND = 67, MOD = 68, DOLLAR = 69, 
    COMMA = 70, SEMI = 71, COLON = 72, DOUBLE_COLON = 73, STAR = 74, DIVIDE = 75, 
    PLUS = 76, MINUS = 77, BIT_NOT = 78, BIT_OR = 79, BIT_XOR = 80, SPACE = 81, 
    COMMENT = 82, LINE_COMMENT1 = 83, LINE_COMMENT2 = 84
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

