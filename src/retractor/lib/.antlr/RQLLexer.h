
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
    ONESHOT = 38, ON = 39, WHEN = 40, DUMP = 41, SYSTEM = 42, DO = 43, TO = 44, 
    AND_C = 45, OR_C = 46, NOT_C = 47, MIN = 48, MAX = 49, AVG = 50, SUMC = 51, 
    STRING_SUBSTRAT = 52, ID = 53, STRING = 54, FLOAT = 55, DECIMAL = 56, 
    REAL = 57, IS_EQ = 58, IS_NQ = 59, IS_GR = 60, IS_LS = 61, IS_GE = 62, 
    IS_LE = 63, EXCLAMATION = 64, DOUBLE_BAR = 65, DOT = 66, UNDERLINE = 67, 
    AT = 68, SHARP = 69, AND = 70, MOD = 71, DOLLAR = 72, COMMA = 73, SEMI = 74, 
    COLON = 75, DOUBLE_COLON = 76, STAR = 77, DIVIDE = 78, PLUS = 79, MINUS = 80, 
    BIT_NOT = 81, BIT_OR = 82, BIT_XOR = 83, SPACE = 84, COMMENT = 85, LINE_COMMENT1 = 86, 
    LINE_COMMENT2 = 87
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

