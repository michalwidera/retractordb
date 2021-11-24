
// Generated from RQLParser.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"




class  RQLParser : public antlr4::Parser {
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

  enum {
    RuleClauses = 0, RuleSelect_statement = 1, RuleDeclare_statement = 2, 
    RuleFull_column_name_list = 3, RuleSelect_list = 4, RuleStream_expression = 5
  };

  explicit RQLParser(antlr4::TokenStream *input);
  ~RQLParser();

  virtual std::string getGrammarFileName() const override;
  virtual const antlr4::atn::ATN& getATN() const override { return _atn; };
  virtual const std::vector<std::string>& getTokenNames() const override { return _tokenNames; }; // deprecated: use vocabulary instead.
  virtual const std::vector<std::string>& getRuleNames() const override;
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;


  class ClausesContext;
  class Select_statementContext;
  class Declare_statementContext;
  class Full_column_name_listContext;
  class Select_listContext;
  class Stream_expressionContext; 

  class  ClausesContext : public antlr4::ParserRuleContext {
  public:
    ClausesContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Select_statementContext *select_statement();
    Declare_statementContext *declare_statement();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ClausesContext* clauses();

  class  Select_statementContext : public antlr4::ParserRuleContext {
  public:
    RQLParser::Select_listContext *columns = nullptr;
    RQLParser::Stream_expressionContext *from = nullptr;
    Select_statementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *SELECT();
    antlr4::tree::TerminalNode *STREAM();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *FROM();
    Select_listContext *select_list();
    Stream_expressionContext *stream_expression();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Select_statementContext* select_statement();

  class  Declare_statementContext : public antlr4::ParserRuleContext {
  public:
    Declare_statementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DECLARE();
    Full_column_name_listContext *full_column_name_list();
    antlr4::tree::TerminalNode *STREAM();
    std::vector<antlr4::tree::TerminalNode *> ID();
    antlr4::tree::TerminalNode* ID(size_t i);
    antlr4::tree::TerminalNode *FILE();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Declare_statementContext* declare_statement();

  class  Full_column_name_listContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *idToken = nullptr;
    std::vector<antlr4::Token *> column;
    Full_column_name_listContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> ID();
    antlr4::tree::TerminalNode* ID(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Full_column_name_listContext* full_column_name_list();

  class  Select_listContext : public antlr4::ParserRuleContext {
  public:
    Select_listContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ID();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Select_listContext* select_list();

  class  Stream_expressionContext : public antlr4::ParserRuleContext {
  public:
    Stream_expressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ID();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Stream_expressionContext* stream_expression();


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

