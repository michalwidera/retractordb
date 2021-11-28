
    #include <iostream>


// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"




class  RQLLexer : public antlr4::Lexer {
public:
  enum {
    STRING_T = 1, INTEGER_T = 2, BYTE_T = 3, UNSIGNED_T = 4, FLOAT_T = 5, 
    DOUBLE_T = 6, SELECT = 7, STREAM = 8, FROM = 9, DECLARE = 10, FILE = 11, 
    MIN = 12, MAX = 13, AVG = 14, SUMC = 15, SQRT = 16, CEIL = 17, ABS = 18, 
    FLOOR = 19, SIGN = 20, CHR = 21, LENGTH = 22, TO_NUMBER = 23, TO_TIMESTAMP = 24, 
    FLOAT_CAST = 25, INT_CAST = 26, COUNT = 27, CRC = 28, SUM = 29, ISZERO = 30, 
    ISNONZERO = 31, ID = 32, STRING = 33, FLOAT = 34, DECIMAL = 35, RATIONAL = 36, 
    REAL = 37, EQUAL = 38, GREATER = 39, LESS = 40, EXCLAMATION = 41, DOUBLE_BAR = 42, 
    DOT = 43, UNDERLINE = 44, AT = 45, SHARP = 46, AND = 47, MOD = 48, DOLLAR = 49, 
    LR_BRACKET = 50, RR_BRACKET = 51, LS_BRACKET = 52, RS_BRACKET = 53, 
    LC_BRACKET = 54, RC_BRACKET = 55, COMMA = 56, SEMI = 57, COLON = 58, 
    DOUBLE_COLON = 59, STAR = 60, DIVIDE = 61, PLUS = 62, MINUS = 63, BIT_NOT = 64, 
    BIT_OR = 65, BIT_XOR = 66, SPACE = 67, COMMENT = 68, LINE_COMMENT = 69
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

