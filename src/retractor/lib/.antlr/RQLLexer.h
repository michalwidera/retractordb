
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
    ONESHOT = 38, HOLD = 39, ON = 40, WHEN = 41, DUMP = 42, SYSTEM = 43, 
    DO = 44, TO = 45, AND_C = 46, OR_C = 47, NOT_C = 48, MIN = 49, MAX = 50, 
    AVG = 51, SUMC = 52, STRING_SUBSTRAT = 53, ID = 54, STRING = 55, FLOAT = 56, 
    DECIMAL = 57, REAL = 58, IS_EQ = 59, IS_NQ = 60, IS_GR = 61, IS_LS = 62, 
    IS_GE = 63, IS_LE = 64, EXCLAMATION = 65, DOUBLE_BAR = 66, DOT = 67, 
    UNDERLINE = 68, AT = 69, SHARP = 70, AND = 71, MOD = 72, DOLLAR = 73, 
    COMMA = 74, SEMI = 75, COLON = 76, DOUBLE_COLON = 77, STAR = 78, DIVIDE = 79, 
    PLUS = 80, MINUS = 81, BIT_NOT = 82, BIT_OR = 83, BIT_XOR = 84, SPACE = 85, 
    COMMENT = 86, LINE_COMMENT1 = 87, LINE_COMMENT2 = 88
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

