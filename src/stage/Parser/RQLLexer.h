
    // https://github.com/antlr/grammars-v4/blob/master/sql/tsql/TSqlParser.g4
    #include <iostream>


// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"




class  RQLLexer : public antlr4::Lexer {
public:
  enum {
    SELECT = 1, STREAM = 2, FROM = 3, DECLARE = 4, FILE = 5, MIN = 6, MAX = 7, 
    AVG = 8, SUMC = 9, ID = 10, STRING = 11, FLOAT = 12, DECIMAL = 13, SIGNED_DECIMAL = 14, 
    RATIONAL = 15, REAL = 16, EQUAL = 17, GREATER = 18, LESS = 19, EXCLAMATION = 20, 
    DOUBLE_BAR = 21, DOT = 22, UNDERLINE = 23, AT = 24, SHARP = 25, AND = 26, 
    MOD = 27, DOLLAR = 28, LR_BRACKET = 29, RR_BRACKET = 30, COMMA = 31, 
    SEMI = 32, COLON = 33, DOUBLE_COLON = 34, STAR = 35, DIVIDE = 36, PLUS = 37, 
    MINUS = 38, BIT_NOT = 39, BIT_OR = 40, BIT_XOR = 41, SPACE = 42, COMMENT = 43, 
    LINE_COMMENT = 44
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

