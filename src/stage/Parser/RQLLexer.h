
// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"




class  RQLLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, STRING_T = 5, BYTEARRAY_T = 6, 
    INTARRAY_T = 7, BYTE_T = 8, UNSIGNED_T = 9, INTEGER_T = 10, FLOAT_T = 11, 
    DOUBLE_T = 12, SELECT = 13, STREAM = 14, FROM = 15, DECLARE = 16, FILE = 17, 
    MIN = 18, MAX = 19, AVG = 20, SUMC = 21, ID = 22, STRING = 23, FLOAT = 24, 
    DECIMAL = 25, REAL = 26, FUNCTION = 27, EQUAL = 28, GREATER = 29, LESS = 30, 
    EXCLAMATION = 31, DOUBLE_BAR = 32, DOT = 33, UNDERLINE = 34, AT = 35, 
    SHARP = 36, AND = 37, MOD = 38, DOLLAR = 39, COMMA = 40, SEMI = 41, 
    COLON = 42, DOUBLE_COLON = 43, STAR = 44, DIVIDE = 45, PLUS = 46, MINUS = 47, 
    BIT_NOT = 48, BIT_OR = 49, BIT_XOR = 50, SPACE = 51, COMMENT = 52, LINE_COMMENT = 53
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

