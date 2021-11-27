
    // https://github.com/antlr/grammars-v4/blob/master/sql/tsql/TSqlParser.g4
    #include <iostream>


// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"




class  RQLParser : public antlr4::Parser {
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

  enum {
    RuleProg = 0, RuleSelect_statement = 1, RuleDeclare_statement = 2, RuleColumn_name_list = 3, 
    RuleSelect_list = 4, RuleStream_expression = 5, RuleStream_term = 6, 
    RuleAgregator = 7
  };

  explicit RQLParser(antlr4::TokenStream *input);
  ~RQLParser();

  virtual std::string getGrammarFileName() const override;
  virtual const antlr4::atn::ATN& getATN() const override { return _atn; };
  virtual const std::vector<std::string>& getTokenNames() const override { return _tokenNames; }; // deprecated: use vocabulary instead.
  virtual const std::vector<std::string>& getRuleNames() const override;
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;


  class ProgContext;
  class Select_statementContext;
  class Declare_statementContext;
  class Column_name_listContext;
  class Select_listContext;
  class Stream_expressionContext;
  class Stream_termContext;
  class AgregatorContext; 

  class  ProgContext : public antlr4::ParserRuleContext {
  public:
    ProgContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *EOF();
    std::vector<Select_statementContext *> select_statement();
    Select_statementContext* select_statement(size_t i);
    std::vector<Declare_statementContext *> declare_statement();
    Declare_statementContext* declare_statement(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ProgContext* prog();

  class  Select_statementContext : public antlr4::ParserRuleContext {
  public:
    RQLParser::Select_listContext *columns = nullptr;
    antlr4::Token *id = nullptr;
    RQLParser::Stream_expressionContext *from = nullptr;
    Select_statementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SELECT();
    antlr4::tree::TerminalNode *STREAM();
    antlr4::tree::TerminalNode *FROM();
    Select_listContext *select_list();
    antlr4::tree::TerminalNode *ID();
    Stream_expressionContext *stream_expression();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Select_statementContext* select_statement();

  class  Declare_statementContext : public antlr4::ParserRuleContext {
  public:
    Declare_statementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DECLARE();
    Column_name_listContext *column_name_list();
    antlr4::tree::TerminalNode *STREAM();
    std::vector<antlr4::tree::TerminalNode *> ID();
    antlr4::tree::TerminalNode* ID(size_t i);
    antlr4::tree::TerminalNode *FILE();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Declare_statementContext* declare_statement();

  class  Column_name_listContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *idToken = nullptr;
    std::vector<antlr4::Token *> column;
    Column_name_listContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> ID();
    antlr4::tree::TerminalNode* ID(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Column_name_listContext* column_name_list();

  class  Select_listContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *idToken = nullptr;
    std::vector<antlr4::Token *> column;
    Select_listContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> ID();
    antlr4::tree::TerminalNode* ID(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Select_listContext* select_list();

  class  Stream_expressionContext : public antlr4::ParserRuleContext {
  public:
    Stream_expressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<Stream_termContext *> stream_term();
    Stream_termContext* stream_term(size_t i);
    std::vector<antlr4::tree::TerminalNode *> GREATER();
    antlr4::tree::TerminalNode* GREATER(size_t i);
    std::vector<antlr4::tree::TerminalNode *> DECIMAL();
    antlr4::tree::TerminalNode* DECIMAL(size_t i);
    std::vector<antlr4::tree::TerminalNode *> PLUS();
    antlr4::tree::TerminalNode* PLUS(size_t i);
    std::vector<antlr4::tree::TerminalNode *> MINUS();
    antlr4::tree::TerminalNode* MINUS(size_t i);
    std::vector<antlr4::tree::TerminalNode *> RATIONAL();
    antlr4::tree::TerminalNode* RATIONAL(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Stream_expressionContext* stream_expression();

  class  Stream_termContext : public antlr4::ParserRuleContext {
  public:
    Stream_termContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> ID();
    antlr4::tree::TerminalNode* ID(size_t i);
    std::vector<antlr4::tree::TerminalNode *> SHARP();
    antlr4::tree::TerminalNode* SHARP(size_t i);
    std::vector<antlr4::tree::TerminalNode *> AND();
    antlr4::tree::TerminalNode* AND(size_t i);
    std::vector<antlr4::tree::TerminalNode *> RATIONAL();
    antlr4::tree::TerminalNode* RATIONAL(size_t i);
    std::vector<antlr4::tree::TerminalNode *> MOD();
    antlr4::tree::TerminalNode* MOD(size_t i);
    std::vector<antlr4::tree::TerminalNode *> AT();
    antlr4::tree::TerminalNode* AT(size_t i);
    std::vector<antlr4::tree::TerminalNode *> LR_BRACKET();
    antlr4::tree::TerminalNode* LR_BRACKET(size_t i);
    std::vector<antlr4::tree::TerminalNode *> DECIMAL();
    antlr4::tree::TerminalNode* DECIMAL(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);
    std::vector<antlr4::tree::TerminalNode *> SIGNED_DECIMAL();
    antlr4::tree::TerminalNode* SIGNED_DECIMAL(size_t i);
    std::vector<antlr4::tree::TerminalNode *> RR_BRACKET();
    antlr4::tree::TerminalNode* RR_BRACKET(size_t i);
    std::vector<antlr4::tree::TerminalNode *> DOT();
    antlr4::tree::TerminalNode* DOT(size_t i);
    std::vector<AgregatorContext *> agregator();
    AgregatorContext* agregator(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Stream_termContext* stream_term();

  class  AgregatorContext : public antlr4::ParserRuleContext {
  public:
    AgregatorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *MIN();
    antlr4::tree::TerminalNode *MAX();
    antlr4::tree::TerminalNode *AVG();
    antlr4::tree::TerminalNode *SUMC();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AgregatorContext* agregator();


private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

