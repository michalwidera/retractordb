
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
    INTEGER_T = 26, FLOAT_T = 27, DOUBLE_T = 28, SELECT = 29, STREAM = 30, 
    FROM = 31, DECLARE = 32, FILE = 33, MIN = 34, MAX = 35, AVG = 36, SUMC = 37, 
    ID = 38, STRING = 39, FLOAT = 40, DECIMAL = 41, REAL = 42, EQUAL = 43, 
    GREATER = 44, LESS = 45, EXCLAMATION = 46, DOUBLE_BAR = 47, DOT = 48, 
    UNDERLINE = 49, AT = 50, SHARP = 51, AND = 52, MOD = 53, DOLLAR = 54, 
    COMMA = 55, SEMI = 56, COLON = 57, DOUBLE_COLON = 58, STAR = 59, DIVIDE = 60, 
    PLUS = 61, MINUS = 62, BIT_NOT = 63, BIT_OR = 64, BIT_XOR = 65, SPACE = 66, 
    COMMENT = 67, LINE_COMMENT = 68
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

