
// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"




class  RQLLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, STRING_T = 5, INTEGER_T = 6, 
    BYTE_T = 7, UNSIGNED_T = 8, FLOAT_T = 9, DOUBLE_T = 10, SELECT = 11, 
    STREAM = 12, FROM = 13, DECLARE = 14, FILE = 15, MIN = 16, MAX = 17, 
    AVG = 18, SUMC = 19, SQRT = 20, CEIL = 21, ABS = 22, FLOOR = 23, SIGN = 24, 
    CHR = 25, LENGTH = 26, TO_NUMBER = 27, TO_TIMESTAMP = 28, FLOAT_CAST = 29, 
    INT_CAST = 30, COUNT = 31, CRC = 32, SUM = 33, ISZERO = 34, ISNONZERO = 35, 
    ID = 36, STRING = 37, FLOAT = 38, DECIMAL = 39, RATIONAL = 40, REAL = 41, 
    EQUAL = 42, GREATER = 43, LESS = 44, EXCLAMATION = 45, DOUBLE_BAR = 46, 
    DOT = 47, UNDERLINE = 48, AT = 49, SHARP = 50, AND = 51, MOD = 52, DOLLAR = 53, 
    COMMA = 54, SEMI = 55, COLON = 56, DOUBLE_COLON = 57, STAR = 58, DIVIDE = 59, 
    PLUS = 60, MINUS = 61, BIT_NOT = 62, BIT_OR = 63, BIT_XOR = 64, SPACE = 65, 
    COMMENT = 66, LINE_COMMENT = 67
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

