
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
    DISPOSABLE = 24, ONESHOT = 25, HOLD = 26, VOLATILE = 27, PERSISTENT = 28, 
    DEFAULT = 29, ON = 30, WHEN = 31, DUMP = 32, SYSTEM = 33, DO = 34, TO = 35, 
    AND_C = 36, OR_C = 37, NOT_C = 38, MIN = 39, MAX = 40, AVG = 41, SUMC = 42, 
    TYPE_PROFILE = 43, STRING_PROFILE = 44, ID = 45, STRING = 46, FLOAT = 47, 
    DECIMAL = 48, REAL = 49, IS_EQ = 50, IS_NQ = 51, IS_GR = 52, IS_LS = 53, 
    IS_GE = 54, IS_LE = 55, EXCLAMATION = 56, DOUBLE_BAR = 57, DOT = 58, 
    UNDERLINE = 59, AT = 60, SHARP = 61, AND = 62, MOD = 63, DOLLAR = 64, 
    COMMA = 65, SEMI = 66, COLON = 67, DOUBLE_COLON = 68, STAR = 69, DIVIDE = 70, 
    PLUS = 71, MINUS = 72, BIT_NOT = 73, BIT_OR = 74, BIT_XOR = 75, SPACE = 76, 
    COMMENT = 77, LINE_COMMENT2 = 78
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

