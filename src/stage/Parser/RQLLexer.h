
// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"




class  RQLLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, STRING_T = 5, INTEGER_T = 6, 
    BYTE_T = 7, UNSIGNED_T = 8, FLOAT_T = 9, DOUBLE_T = 10, SELECT = 11, 
    STREAM = 12, FROM = 13, DECLARE = 14, FILE = 15, MIN = 16, MAX = 17, 
    AVG = 18, SUMC = 19, ID = 20, STRING = 21, FLOAT = 22, DECIMAL = 23, 
    REAL = 24, FUNCTION = 25, EQUAL = 26, GREATER = 27, LESS = 28, EXCLAMATION = 29, 
    DOUBLE_BAR = 30, DOT = 31, UNDERLINE = 32, AT = 33, SHARP = 34, AND = 35, 
    MOD = 36, DOLLAR = 37, COMMA = 38, SEMI = 39, COLON = 40, DOUBLE_COLON = 41, 
    STAR = 42, DIVIDE = 43, PLUS = 44, MINUS = 45, BIT_NOT = 46, BIT_OR = 47, 
    BIT_XOR = 48, SPACE = 49, COMMENT = 50, LINE_COMMENT = 51
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

