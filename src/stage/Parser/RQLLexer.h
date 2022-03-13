
// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"




class  RQLLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, T__18 = 19, T__19 = 20, 
    STRING_T = 21, BYTEARRAY_T = 22, INTARRAY_T = 23, BYTE_T = 24, UNSIGNED_T = 25, 
    INTEGER_T = 26, FLOAT_T = 27, SELECT = 28, STREAM = 29, FROM = 30, DECLARE = 31, 
    FILE = 32, MIN = 33, MAX = 34, AVG = 35, SUMC = 36, ID = 37, STRING = 38, 
    FLOAT = 39, DECIMAL = 40, REAL = 41, EQUAL = 42, GREATER = 43, LESS = 44, 
    EXCLAMATION = 45, DOUBLE_BAR = 46, DOT = 47, UNDERLINE = 48, AT = 49, 
    SHARP = 50, AND = 51, MOD = 52, DOLLAR = 53, COMMA = 54, SEMI = 55, 
    COLON = 56, DOUBLE_COLON = 57, STAR = 58, DIVIDE = 59, PLUS = 60, MINUS = 61, 
    BIT_NOT = 62, BIT_OR = 63, BIT_XOR = 64, SPACE = 65, COMMENT = 66, LINE_COMMENT = 67
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

