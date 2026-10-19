
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
    PERSISTENT = 29, DEFAULT = 30, LET = 31, VAR = 32, ON = 33, WHEN = 34, 
    DUMP = 35, SYSTEM = 36, SET = 37, DO = 38, TO = 39, AND_C = 40, OR_C = 41, 
    NOT_C = 42, MIN = 43, MAX = 44, AVG = 45, SUMC = 46, TYPE_PROFILE = 47, 
    STRING_PROFILE = 48, ID = 49, CONST_REF = 50, STRING = 51, FLOAT = 52, 
    DECIMAL = 53, REAL = 54, IS_EQ = 55, IS_NQ = 56, IS_GR = 57, IS_LS = 58, 
    IS_GE = 59, IS_LE = 60, EXCLAMATION = 61, DOUBLE_BAR = 62, DOT = 63, 
    UNDERLINE = 64, AT = 65, SHARP = 66, AND = 67, MOD = 68, DOLLAR = 69, 
    COMMA = 70, SEMI = 71, COLON = 72, DOUBLE_COLON = 73, STAR = 74, DIVIDE = 75, 
    PLUS = 76, MINUS = 77, BIT_NOT = 78, BIT_OR = 79, BIT_XOR = 80, SPACE = 81, 
    COMMENT = 82, LINE_COMMENT2 = 83
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

