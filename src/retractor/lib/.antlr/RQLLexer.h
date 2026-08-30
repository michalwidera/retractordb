
// Generated from RQL.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  RQLLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, BYTE_T = 5, STRING_T = 6, UNSIGNED_T = 7, 
    INTEGER_T = 8, FLOAT_T = 9, DOUBLE_T = 10, SELECT = 11, STREAM = 12, 
    FROM = 13, DECLARE = 14, RETENTION = 15, FILE = 16, STORAGE = 17, ROTATION = 18, 
    SUBSTRAT = 19, RULE = 20, DISPOSABLE = 21, ONESHOT = 22, HOLD = 23, 
    VOLATILE = 24, ON = 25, WHEN = 26, DUMP = 27, SYSTEM = 28, DO = 29, 
    TO = 30, AND_C = 31, OR_C = 32, NOT_C = 33, MIN = 34, MAX = 35, AVG = 36, 
    SUMC = 37, TYPE_PROFILE = 38, STRING_PROFILE = 39, ID = 40, STRING = 41, 
    FLOAT = 42, DECIMAL = 43, REAL = 44, IS_EQ = 45, IS_NQ = 46, IS_GR = 47, 
    IS_LS = 48, IS_GE = 49, IS_LE = 50, EXCLAMATION = 51, DOUBLE_BAR = 52, 
    DOT = 53, UNDERLINE = 54, AT = 55, SHARP = 56, AND = 57, MOD = 58, DOLLAR = 59, 
    COMMA = 60, SEMI = 61, COLON = 62, DOUBLE_COLON = 63, STAR = 64, DIVIDE = 65, 
    PLUS = 66, MINUS = 67, BIT_NOT = 68, BIT_OR = 69, BIT_XOR = 70, SPACE = 71, 
    COMMENT = 72, LINE_COMMENT2 = 73
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

