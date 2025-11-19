
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
    FILE = 32, STORAGE = 33, ROTATION = 34, SUBSTRAT = 35, RULE = 36, ON = 37, 
    WHEN = 38, DUMP = 39, SYSTEM = 40, DO = 41, TO = 42, AND_C = 43, OR_C = 44, 
    NOT_C = 45, MIN = 46, MAX = 47, AVG = 48, SUMC = 49, STRING_SUBSTRAT = 50, 
    ID = 51, STRING = 52, FLOAT = 53, DECIMAL = 54, REAL = 55, IS_EQ = 56, 
    IS_NQ = 57, IS_GR = 58, IS_LS = 59, IS_GE = 60, IS_LE = 61, EXCLAMATION = 62, 
    DOUBLE_BAR = 63, DOT = 64, UNDERLINE = 65, AT = 66, SHARP = 67, AND = 68, 
    MOD = 69, DOLLAR = 70, COMMA = 71, SEMI = 72, COLON = 73, DOUBLE_COLON = 74, 
    STAR = 75, DIVIDE = 76, PLUS = 77, MINUS = 78, BIT_NOT = 79, BIT_OR = 80, 
    BIT_XOR = 81, SPACE = 82, COMMENT = 83, LINE_COMMENT1 = 84, LINE_COMMENT2 = 85
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

