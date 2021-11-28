
    #include <iostream>


// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"




class  RQLLexer : public antlr4::Lexer {
public:
  enum {
    FLOAT_T = 1, STRING_T = 2, BYTE_T = 3, INTEGER_T = 4, UNSIGNED_T = 5, 
    ARRAY_B_T = 6, ARRAY_I_T = 7, SELECT = 8, STREAM = 9, FROM = 10, DECLARE = 11, 
    FILE = 12, MIN = 13, MAX = 14, AVG = 15, SUMC = 16, SQRT = 17, CEIL = 18, 
    ABS = 19, FLOOR = 20, SIGN = 21, CHR = 22, LENGTH = 23, TO_NUMBER = 24, 
    TO_TIMESTAMP = 25, FLOAT_CAST = 26, INT_CAST = 27, COUNT = 28, CRC = 29, 
    SUM = 30, ISZERO = 31, ISNONZERO = 32, ID = 33, STRING = 34, FLOAT = 35, 
    DECIMAL = 36, RATIONAL = 37, REAL = 38, EQUAL = 39, GREATER = 40, LESS = 41, 
    EXCLAMATION = 42, DOUBLE_BAR = 43, DOT = 44, UNDERLINE = 45, AT = 46, 
    SHARP = 47, AND = 48, MOD = 49, DOLLAR = 50, LR_BRACKET = 51, RR_BRACKET = 52, 
    LS_BRACKET = 53, RS_BRACKET = 54, LC_BRACKET = 55, RC_BRACKET = 56, 
    COMMA = 57, SEMI = 58, COLON = 59, DOUBLE_COLON = 60, STAR = 61, DIVIDE = 62, 
    PLUS = 63, MINUS = 64, BIT_NOT = 65, BIT_OR = 66, BIT_XOR = 67, SPACE = 68, 
    COMMENT = 69, LINE_COMMENT = 70
  };

  explicit RQLLexer(antlr4::CharStream *input);
  ~RQLLexer();

  virtual std::string getGrammarFileName() const override;
  virtual const std::vector<std::string>& getRuleNames() const override;

  virtual const std::vector<std::string>& getChannelNames() const override;
  virtual const std::vector<std::string>& getModeNames() const override;
  virtual const std::vector<std::string>& getTokenNames() const override; // deprecated, use vocabulary instead
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;

  virtual const std::vector<uint16_t> getSerializedATN() const override;
  virtual const antlr4::atn::ATN& getATN() const override;

private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;
  static std::vector<std::string> _channelNames;
  static std::vector<std::string> _modeNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

