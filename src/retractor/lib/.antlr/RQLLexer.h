
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
    FILE = 32, STORAGE = 33, ROTATION = 34, SUBSTRAT = 35, RULE = 36, DISPOSABLE = 37, 
    ONESHOT = 38, HOLD = 39, VOLATILE = 40, ON = 41, WHEN = 42, DUMP = 43, 
    SYSTEM = 44, DO = 45, TO = 46, AND_C = 47, OR_C = 48, NOT_C = 49, MIN = 50, 
    MAX = 51, AVG = 52, SUMC = 53, STRING_PROFILE = 54, ID = 55, STRING = 56, 
    FLOAT = 57, DECIMAL = 58, REAL = 59, IS_EQ = 60, IS_NQ = 61, IS_GR = 62, 
    IS_LS = 63, IS_GE = 64, IS_LE = 65, EXCLAMATION = 66, DOUBLE_BAR = 67, 
    DOT = 68, UNDERLINE = 69, AT = 70, SHARP = 71, AND = 72, MOD = 73, DOLLAR = 74, 
    COMMA = 75, SEMI = 76, COLON = 77, DOUBLE_COLON = 78, STAR = 79, DIVIDE = 80, 
    PLUS = 81, MINUS = 82, BIT_NOT = 83, BIT_OR = 84, BIT_XOR = 85, SPACE = 86, 
    COMMENT = 87, LINE_COMMENT1 = 88, LINE_COMMENT2 = 89
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

