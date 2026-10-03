
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
    PERSISTENT = 29, DEFAULT = 30, ON = 31, WHEN = 32, DUMP = 33, SYSTEM = 34, 
    DO = 35, TO = 36, AND_C = 37, OR_C = 38, NOT_C = 39, MIN = 40, MAX = 41, 
    AVG = 42, SUMC = 43, TYPE_PROFILE = 44, STRING_PROFILE = 45, ID = 46, 
    STRING = 47, FLOAT = 48, DECIMAL = 49, REAL = 50, IS_EQ = 51, IS_NQ = 52, 
    IS_GR = 53, IS_LS = 54, IS_GE = 55, IS_LE = 56, EXCLAMATION = 57, DOUBLE_BAR = 58, 
    DOT = 59, UNDERLINE = 60, AT = 61, SHARP = 62, AND = 63, MOD = 64, DOLLAR = 65, 
    COMMA = 66, SEMI = 67, COLON = 68, DOUBLE_COLON = 69, STAR = 70, DIVIDE = 71, 
    PLUS = 72, MINUS = 73, BIT_NOT = 74, BIT_OR = 75, BIT_XOR = 76, SPACE = 77, 
    COMMENT = 78, LINE_COMMENT2 = 79
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

