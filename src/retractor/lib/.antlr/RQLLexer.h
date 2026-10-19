
// Generated from RQL.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"




class  RQLLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, BYTE_T = 5, STRING_T = 6, UNSIGNED_T = 7, 
    INTEGER_T = 8, FLOAT_T = 9, DOUBLE_T = 10, SELECT = 11, STREAM = 12, 
    FROM = 13, DECLARE = 14, RETENTION = 15, FILE = 16, BINFILE = 17, TEXTFILE = 18, 
    DEVICE = 19, STORAGE = 20, ROTATION = 21, SUBSTRAT = 22, RULE = 23, 
    DISPOSABLE = 24, ONESHOT = 25, HOLD = 26, TIMEOUT = 27, VOLATILE = 28, 
    PERSISTENT = 29, DEFAULT = 30, LET = 31, ON = 32, WHEN = 33, DUMP = 34, 
    SYSTEM = 35, DO = 36, TO = 37, AND_C = 38, OR_C = 39, NOT_C = 40, MIN = 41, 
    MAX = 42, AVG = 43, SUMC = 44, TYPE_PROFILE = 45, STRING_PROFILE = 46, 
    ID = 47, CONST_REF = 48, STRING = 49, FLOAT = 50, DECIMAL = 51, REAL = 52, 
    IS_EQ = 53, IS_NQ = 54, IS_GR = 55, IS_LS = 56, IS_GE = 57, IS_LE = 58, 
    EXCLAMATION = 59, DOUBLE_BAR = 60, DOT = 61, UNDERLINE = 62, AT = 63, 
    SHARP = 64, AND = 65, MOD = 66, DOLLAR = 67, COMMA = 68, SEMI = 69, 
    COLON = 70, DOUBLE_COLON = 71, STAR = 72, DIVIDE = 73, PLUS = 74, MINUS = 75, 
    BIT_NOT = 76, BIT_OR = 77, BIT_XOR = 78, SPACE = 79, COMMENT = 80, LINE_COMMENT2 = 81
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

