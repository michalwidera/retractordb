
// Generated from RQL.g4 by ANTLR 4.13.1

#pragma once

#include "antlr4-runtime.h"

class RQLLexer : public antlr4::Lexer {
 public:
  enum {
    T__0          = 1,
    T__1          = 2,
    T__2          = 3,
    T__3          = 4,
    T__4          = 5,
    T__5          = 6,
    T__6          = 7,
    T__7          = 8,
    T__8          = 9,
    T__9          = 10,
    T__10         = 11,
    T__11         = 12,
    T__12         = 13,
    T__13         = 14,
    T__14         = 15,
    T__15         = 16,
    T__16         = 17,
    T__17         = 18,
    T__18         = 19,
    T__19         = 20,
    BYTE_T        = 21,
    STRING_T      = 22,
    UNSIGNED_T    = 23,
    INTEGER_T     = 24,
    FLOAT_T       = 25,
    DOUBLE_T      = 26,
    SELECT        = 27,
    STREAM        = 28,
    FROM          = 29,
    DECLARE       = 30,
    RETENTION     = 31,
    FILE          = 32,
    STORAGE       = 33,
    MIN           = 34,
    MAX           = 35,
    AVG           = 36,
    SUMC          = 37,
    ID            = 38,
    STRING        = 39,
    FLOAT         = 40,
    DECIMAL       = 41,
    REAL          = 42,
    EQUAL         = 43,
    GREATER       = 44,
    LESS          = 45,
    EXCLAMATION   = 46,
    DOUBLE_BAR    = 47,
    DOT           = 48,
    UNDERLINE     = 49,
    AT            = 50,
    SHARP         = 51,
    AND           = 52,
    MOD           = 53,
    DOLLAR        = 54,
    COMMA         = 55,
    SEMI          = 56,
    COLON         = 57,
    DOUBLE_COLON  = 58,
    STAR          = 59,
    DIVIDE        = 60,
    PLUS          = 61,
    MINUS         = 62,
    BIT_NOT       = 63,
    BIT_OR        = 64,
    BIT_XOR       = 65,
    SPACE         = 66,
    COMMENT       = 67,
    LINE_COMMENT1 = 68,
    LINE_COMMENT2 = 69
  };

  explicit RQLLexer(antlr4::CharStream *input);

  ~RQLLexer() override;

  std::string getGrammarFileName() const override;

  const std::vector<std::string> &getRuleNames() const override;

  const std::vector<std::string> &getChannelNames() const override;

  const std::vector<std::string> &getModeNames() const override;

  const antlr4::dfa::Vocabulary &getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN &getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

 private:
  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.
};
