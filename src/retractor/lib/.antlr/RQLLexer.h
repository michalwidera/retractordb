
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
    STRING_PROFILE = 56, TO_INTEGER_FN = 57, TO_FLOAT_FN = 58, TO_DOUBLE_FN = 59, 
    TO_STRING_FN = 60, ID = 61, STRING = 62, FLOAT = 63, DECIMAL = 64, REAL = 65, 
    IS_EQ = 66, IS_NQ = 67, IS_GR = 68, IS_LS = 69, IS_GE = 70, IS_LE = 71, 
    EXCLAMATION = 72, DOUBLE_BAR = 73, DOT = 74, UNDERLINE = 75, AT = 76, 
    SHARP = 77, AND = 78, MOD = 79, DOLLAR = 80, COMMA = 81, SEMI = 82, 
    COLON = 83, DOUBLE_COLON = 84, STAR = 85, DIVIDE = 86, PLUS = 87, MINUS = 88, 
    BIT_NOT = 89, BIT_OR = 90, BIT_XOR = 91, SPACE = 92, COMMENT = 93, LINE_COMMENT1 = 94, 
    LINE_COMMENT2 = 95
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

