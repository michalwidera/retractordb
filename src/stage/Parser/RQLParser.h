
// @header from RQL.g4
#include <iostream>
// End of @header


// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"




class  RQLParser : public antlr4::Parser {
public:
  enum {
    STRING_T = 1, INTEGER_T = 2, BYTE_T = 3, UNSIGNED_T = 4, FLOAT_T = 5, 
    DOUBLE_T = 6, SELECT = 7, STREAM = 8, FROM = 9, DECLARE = 10, FILE = 11, 
    MIN = 12, MAX = 13, AVG = 14, SUMC = 15, SQRT = 16, CEIL = 17, ABS = 18, 
    FLOOR = 19, SIGN = 20, CHR = 21, LENGTH = 22, TO_NUMBER = 23, TO_TIMESTAMP = 24, 
    FLOAT_CAST = 25, INT_CAST = 26, COUNT = 27, CRC = 28, SUM = 29, ISZERO = 30, 
    ISNONZERO = 31, ID = 32, STRING = 33, FLOAT = 34, DECIMAL = 35, RATIONAL = 36, 
    REAL = 37, EQUAL = 38, GREATER = 39, LESS = 40, EXCLAMATION = 41, DOUBLE_BAR = 42, 
    DOT = 43, UNDERLINE = 44, AT = 45, SHARP = 46, AND = 47, MOD = 48, DOLLAR = 49, 
    LR_BRACKET = 50, RR_BRACKET = 51, LS_BRACKET = 52, RS_BRACKET = 53, 
    LC_BRACKET = 54, RC_BRACKET = 55, COMMA = 56, SEMI = 57, COLON = 58, 
    DOUBLE_COLON = 59, STAR = 60, DIVIDE = 61, PLUS = 62, MINUS = 63, BIT_NOT = 64, 
    BIT_OR = 65, BIT_XOR = 66, SPACE = 67, COMMENT = 68, LINE_COMMENT = 69
  };

  enum {
    RuleProg = 0, RuleSelect_statement = 1, RuleRational = 2, RuleDeclare_statement = 3, 
    RuleColumn_name_list = 4, RuleColumn_type = 5, RuleSelect_list = 6, 
    RuleSelect_list_elem = 7, RuleField_id = 8, RuleUnary_op_expression = 9, 
    RuleAsterisk = 10, RuleExpression = 11, RuleTerm = 12, RuleFactor = 13, 
    RuleStream_expression = 14, RuleStream_term = 15, RuleStream_factor = 16, 
    RuleAgregator = 17, RuleFunction_call = 18
  };

  explicit RQLParser(antlr4::TokenStream *input);
  ~RQLParser();

  virtual std::string getGrammarFileName() const override;
  virtual const antlr4::atn::ATN& getATN() const override { return _atn; };
  virtual const std::vector<std::string>& getTokenNames() const override { return _tokenNames; }; // deprecated: use vocabulary instead.
  virtual const std::vector<std::string>& getRuleNames() const override;
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;


  // @members from RQL.g4
  // End of @members


  class ProgContext;
  class Select_statementContext;
  class RationalContext;
  class Declare_statementContext;
  class Column_name_listContext;
  class Column_typeContext;
  class Select_listContext;
  class Select_list_elemContext;
  class Field_idContext;
  class Unary_op_expressionContext;
  class AsteriskContext;
  class ExpressionContext;
  class TermContext;
  class FactorContext;
  class Stream_expressionContext;
  class Stream_termContext;
  class Stream_factorContext;
  class AgregatorContext;
  class Function_callContext; 

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

  class  RationalContext : public antlr4::ParserRuleContext {
  public:
    RationalContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *FLOAT();
    antlr4::tree::TerminalNode *REAL();
    antlr4::tree::TerminalNode *DECIMAL();
    antlr4::tree::TerminalNode *RATIONAL();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  RationalContext* rational();

  class  Declare_statementContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *stream_name = nullptr;
    Declare_statementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DECLARE();
    Column_name_listContext *column_name_list();
    antlr4::tree::TerminalNode *STREAM();
    antlr4::tree::TerminalNode *COMMA();
    RationalContext *rational();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *FILE();
    antlr4::tree::TerminalNode *STRING();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Declare_statementContext* declare_statement();

  class  Column_name_listContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *idToken = nullptr;
    std::vector<antlr4::Token *> column;
    Column_name_listContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<Column_typeContext *> column_type();
    Column_typeContext* column_type(size_t i);
    std::vector<antlr4::tree::TerminalNode *> ID();
    antlr4::tree::TerminalNode* ID(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Column_name_listContext* column_name_list();

  class  Column_typeContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *type_size = nullptr;
    Column_typeContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *FLOAT_T();
    antlr4::tree::TerminalNode *BYTE_T();
    antlr4::tree::TerminalNode *INTEGER_T();
    antlr4::tree::TerminalNode *UNSIGNED_T();
    antlr4::tree::TerminalNode *LS_BRACKET();
    antlr4::tree::TerminalNode *RS_BRACKET();
    antlr4::tree::TerminalNode *STRING_T();
    antlr4::tree::TerminalNode *DECIMAL();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Column_typeContext* column_type();

  class  Select_listContext : public antlr4::ParserRuleContext {
  public:
    RQLParser::Select_list_elemContext *select_list_elemContext = nullptr;
    std::vector<Select_list_elemContext *> column;
    Select_listContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<Select_list_elemContext *> select_list_elem();
    Select_list_elemContext* select_list_elem(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Select_listContext* select_list();

  class  Select_list_elemContext : public antlr4::ParserRuleContext {
  public:
    Select_list_elemContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    AsteriskContext *asterisk();
    ExpressionContext *expression();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Select_list_elemContext* select_list_elem();

  class  Field_idContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *column_name = nullptr;
    antlr4::Token *tablename = nullptr;
    antlr4::Token *column_index = nullptr;
    Field_idContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> ID();
    antlr4::tree::TerminalNode* ID(size_t i);
    antlr4::tree::TerminalNode *LS_BRACKET();
    antlr4::tree::TerminalNode *UNDERLINE();
    antlr4::tree::TerminalNode *RS_BRACKET();
    antlr4::tree::TerminalNode *DOT();
    antlr4::tree::TerminalNode *DECIMAL();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Field_idContext* field_id();

  class  Unary_op_expressionContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *op = nullptr;
    Unary_op_expressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *BIT_NOT();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *PLUS();
    antlr4::tree::TerminalNode *MINUS();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Unary_op_expressionContext* unary_op_expression();

  class  AsteriskContext : public antlr4::ParserRuleContext {
  public:
    AsteriskContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *STAR();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *DOT();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  AsteriskContext* asterisk();

  class  ExpressionContext : public antlr4::ParserRuleContext {
  public:
    std::string e;
    ExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    TermContext *term();
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *PLUS();
    antlr4::tree::TerminalNode *MINUS();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExpressionContext* expression();
  ExpressionContext* expression(int precedence);
  class  TermContext : public antlr4::ParserRuleContext {
  public:
    std::string e;
    RQLParser::FactorContext *t = nullptr;
    TermContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LR_BRACKET();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *RR_BRACKET();
    FactorContext *factor();
    std::vector<TermContext *> term();
    TermContext* term(size_t i);
    antlr4::tree::TerminalNode *STAR();
    antlr4::tree::TerminalNode *DIVIDE();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TermContext* term();
  TermContext* term(int precedence);
  class  FactorContext : public antlr4::ParserRuleContext {
  public:
    FactorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *FLOAT();
    antlr4::tree::TerminalNode *REAL();
    antlr4::tree::TerminalNode *DECIMAL();
    Unary_op_expressionContext *unary_op_expression();
    Function_callContext *function_call();
    Field_idContext *field_id();
    AgregatorContext *agregator();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FactorContext* factor();

  class  Stream_expressionContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *timemove = nullptr;
    antlr4::Token *stream_add = nullptr;
    antlr4::Token *stream_sub = nullptr;
    Stream_expressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<Stream_termContext *> stream_term();
    Stream_termContext* stream_term(size_t i);
    std::vector<antlr4::tree::TerminalNode *> DECIMAL();
    antlr4::tree::TerminalNode* DECIMAL(size_t i);
    std::vector<RationalContext *> rational();
    RationalContext* rational(size_t i);
    std::vector<antlr4::tree::TerminalNode *> GREATER();
    antlr4::tree::TerminalNode* GREATER(size_t i);
    std::vector<antlr4::tree::TerminalNode *> PLUS();
    antlr4::tree::TerminalNode* PLUS(size_t i);
    std::vector<antlr4::tree::TerminalNode *> MINUS();
    antlr4::tree::TerminalNode* MINUS(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Stream_expressionContext* stream_expression();

  class  Stream_termContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *hash = nullptr;
    antlr4::Token *dehash_div = nullptr;
    antlr4::Token *dehash_mod = nullptr;
    antlr4::Token *agse = nullptr;
    Stream_termContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    Stream_factorContext *stream_factor();
    std::vector<antlr4::tree::TerminalNode *> ID();
    antlr4::tree::TerminalNode* ID(size_t i);
    std::vector<RationalContext *> rational();
    RationalContext* rational(size_t i);
    std::vector<antlr4::tree::TerminalNode *> LR_BRACKET();
    antlr4::tree::TerminalNode* LR_BRACKET(size_t i);
    std::vector<antlr4::tree::TerminalNode *> DECIMAL();
    antlr4::tree::TerminalNode* DECIMAL(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);
    std::vector<antlr4::tree::TerminalNode *> RR_BRACKET();
    antlr4::tree::TerminalNode* RR_BRACKET(size_t i);
    std::vector<antlr4::tree::TerminalNode *> DOT();
    antlr4::tree::TerminalNode* DOT(size_t i);
    std::vector<AgregatorContext *> agregator();
    AgregatorContext* agregator(size_t i);
    std::vector<antlr4::tree::TerminalNode *> SHARP();
    antlr4::tree::TerminalNode* SHARP(size_t i);
    std::vector<antlr4::tree::TerminalNode *> AND();
    antlr4::tree::TerminalNode* AND(size_t i);
    std::vector<antlr4::tree::TerminalNode *> MOD();
    antlr4::tree::TerminalNode* MOD(size_t i);
    std::vector<antlr4::tree::TerminalNode *> AT();
    antlr4::tree::TerminalNode* AT(size_t i);
    std::vector<antlr4::tree::TerminalNode *> PLUS();
    antlr4::tree::TerminalNode* PLUS(size_t i);
    std::vector<antlr4::tree::TerminalNode *> MINUS();
    antlr4::tree::TerminalNode* MINUS(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Stream_termContext* stream_term();

  class  Stream_factorContext : public antlr4::ParserRuleContext {
  public:
    Stream_factorContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *LR_BRACKET();
    Stream_expressionContext *stream_expression();
    antlr4::tree::TerminalNode *RR_BRACKET();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Stream_factorContext* stream_factor();

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

  class  Function_callContext : public antlr4::ParserRuleContext {
  public:
    antlr4::Token *function = nullptr;
    Function_callContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LR_BRACKET();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *RR_BRACKET();
    antlr4::tree::TerminalNode *SQRT();
    antlr4::tree::TerminalNode *CEIL();
    antlr4::tree::TerminalNode *ABS();
    antlr4::tree::TerminalNode *FLOOR();
    antlr4::tree::TerminalNode *SIGN();
    antlr4::tree::TerminalNode *CHR();
    antlr4::tree::TerminalNode *LENGTH();
    antlr4::tree::TerminalNode *TO_NUMBER();
    antlr4::tree::TerminalNode *TO_TIMESTAMP();
    antlr4::tree::TerminalNode *FLOAT_CAST();
    antlr4::tree::TerminalNode *INT_CAST();
    antlr4::tree::TerminalNode *COUNT();
    antlr4::tree::TerminalNode *CRC();
    antlr4::tree::TerminalNode *SUM();
    antlr4::tree::TerminalNode *ISZERO();
    antlr4::tree::TerminalNode *ISNONZERO();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  Function_callContext* function_call();


  virtual bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;
  bool expressionSempred(ExpressionContext *_localctx, size_t predicateIndex);
  bool termSempred(TermContext *_localctx, size_t predicateIndex);

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

