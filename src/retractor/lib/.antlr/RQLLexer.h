
// Generated from RQL.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  RQLLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, T__18 = 19, T__19 = 20, 
    T__20 = 21, BYTE_T = 22, STRING_T = 23, UNSIGNED_T = 24, INTEGER_T = 25, 
    FLOAT_T = 26, DOUBLE_T = 27, SELECT = 28, STREAM = 29, FROM = 30, DECLARE = 31, 
    RETENTION = 32, FILE = 33, STORAGE = 34, ROTATION = 35, SUBSTRAT = 36, 
    RULE = 37, DISPOSABLE = 38, ONESHOT = 39, HOLD = 40, VOLATILE = 41, 
    ON = 42, WHEN = 43, DUMP = 44, SYSTEM = 45, DO = 46, TO = 47, AND_C = 48, 
    OR_C = 49, NOT_C = 50, MIN = 51, MAX = 52, AVG = 53, SUMC = 54, TYPE_PROFILE = 55, 
    STRING_PROFILE = 56, ID = 57, STRING = 58, FLOAT = 59, DECIMAL = 60, 
    REAL = 61, IS_EQ = 62, IS_NQ = 63, IS_GR = 64, IS_LS = 65, IS_GE = 66, 
    IS_LE = 67, EXCLAMATION = 68, DOUBLE_BAR = 69, DOT = 70, UNDERLINE = 71, 
    AT = 72, SHARP = 73, AND = 74, MOD = 75, DOLLAR = 76, COMMA = 77, SEMI = 78, 
    COLON = 79, DOUBLE_COLON = 80, STAR = 81, DIVIDE = 82, PLUS = 83, MINUS = 84, 
    BIT_NOT = 85, BIT_OR = 86, BIT_XOR = 87, SPACE = 88, COMMENT = 89, LINE_COMMENT1 = 90, 
    LINE_COMMENT2 = 91
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

