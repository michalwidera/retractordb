
// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"




class  RQLLexer : public antlr4::Lexer {
public:
  enum {
    SELECT = 1, STREAM = 2, FROM = 3, DECLARE = 4, FILE = 5, ID = 6, STRING = 7, 
    FLOAT = 8, DECIMAL = 9, REAL = 10, EQUAL = 11, GREATER = 12, LESS = 13, 
    EXCLAMATION = 14, DOUBLE_BAR = 15, DOT = 16, UNDERLINE = 17, AT = 18, 
    SHARP = 19, DOLLAR = 20, LR_BRACKET = 21, RR_BRACKET = 22, COMMA = 23, 
    SEMI = 24, COLON = 25, DOUBLE_COLON = 26, STAR = 27, DIVIDE = 28, MODULE = 29, 
    PLUS = 30, MINUS = 31, BIT_NOT = 32, BIT_OR = 33, BIT_AND = 34, BIT_XOR = 35, 
    SPACE = 36, COMMENT = 37, LINE_COMMENT = 38
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

