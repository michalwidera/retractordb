
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
    VOLATILE = 24, PERSISTENT = 25, DEFAULT = 26, ON = 27, WHEN = 28, DUMP = 29, 
    SYSTEM = 30, DO = 31, TO = 32, AND_C = 33, OR_C = 34, NOT_C = 35, MIN = 36, 
    MAX = 37, AVG = 38, SUMC = 39, TYPE_PROFILE = 40, STRING_PROFILE = 41, 
    ID = 42, STRING = 43, FLOAT = 44, DECIMAL = 45, REAL = 46, IS_EQ = 47, 
    IS_NQ = 48, IS_GR = 49, IS_LS = 50, IS_GE = 51, IS_LE = 52, EXCLAMATION = 53, 
    DOUBLE_BAR = 54, DOT = 55, UNDERLINE = 56, AT = 57, SHARP = 58, AND = 59, 
    MOD = 60, DOLLAR = 61, COMMA = 62, SEMI = 63, COLON = 64, DOUBLE_COLON = 65, 
    STAR = 66, DIVIDE = 67, PLUS = 68, MINUS = 69, BIT_NOT = 70, BIT_OR = 71, 
    BIT_XOR = 72, SPACE = 73, COMMENT = 74, LINE_COMMENT2 = 75
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

