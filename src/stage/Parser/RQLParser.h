
    #include <iostream>


// Generated from RQL.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"




class  RQLParser : public antlr4::Parser {
public:
  enum {
    FLOAT_T = 1, STRING_T = 2, BYTE_T = 3, INTEGER_T = 4, UNSIGNED_T = 5, 
    ARRAY_B_T = 6, ARRAY_I_T = 7, SELECT = 8, STREAM = 9, FROM = 10, DECLARE = 11, 
    FILE = 12, MIN = 13, MAX = 14, AVG = 15, SUMC = 16, SQRT = 17, CEIL = 18, 
    ABS = 19, FLOOR = 20, SIGN = 21, CHR = 22, LENGTH = 23, TO_NUMBER = 24, 
    TO_TIMESTAMP = 25, FLOAT_CAST = 26, INT_CAST = 27, COUNT = 28, CRC = 29, 
    SUM = 30, ISZERO = 31, ISNONZERO = 32, ID = 33, STRING = 34, FLOAT = 35, 
    DECIMAL = 36, RATIONAL = 37, REAL = 38, EQUAL = 39, GREATER = 40, LESS = 41, 
    EXCLAMATION = 42, DOUBLE_BAR = 43, DOT = 44, UNDERLINE = 45, AT = 46, 
    SHARP = 47, AND = 48, MOD = 49, DOLLAR = 50, LR_BRACKET = 51, RR_BRACKET = 52, 
    LS_BRACKET = 53, RS_BRACKET = 54, LC_BRACKET = 55, RC_BRACKET = 56, 
    COMMA = 57, SEMI = 58, COLON = 59, DOUBLE_COLON = 60, STAR = 61, DIVIDE = 62, 
    PLUS = 63, MINUS = 64, BIT_NOT = 65, BIT_OR = 66, BIT_XOR = 67, SPACE = 68, 
    COMMENT = 69, LINE_COMMENT = 70
  };

  enum {
    RuleProg = 0, RuleSelect_statement = 1, RuleDeclare_statement = 2, RuleColumn_name_list = 3, 
    RuleColumn_type = 4, RuleSelect_list = 5, RuleSelect_list_elem = 6, 
    RuleField_id = 7, RuleUnary_op_expression = 8, RuleAsterisk = 9, RuleExpression = 10, 
    RuleTerm = 11, RuleFactor = 12, RuleStream_expression = 13, RuleStream_term = 14, 
    RuleAgregator = 15, RuleFunction_call = 16
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

  class  Declare_statementContext : public antlr4::ParserRuleContext {
  public:
    Declare_statementContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *DECLARE();
    Column_name_listContext *column_name_list();
    antlr4::tree::TerminalNode *STREAM();
    antlr4::tree::TerminalNode *ID();
    antlr4::tree::TerminalNode *COMMA();
    antlr4::tree::TerminalNode *DECIMAL();
    antlr4::tree::TerminalNode *FLOAT();
    antlr4::tree::TerminalNode *RATIONAL();
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
    antlr4::tree::TerminalNode *ARRAY_B_T();
    antlr4::tree::TerminalNode *ARRAY_I_T();
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
    ExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<TermContext *> term();
    TermContext* term(size_t i);
    std::vector<antlr4::tree::TerminalNode *> PLUS();
    antlr4::tree::TerminalNode* PLUS(size_t i);
    std::vector<antlr4::tree::TerminalNode *> MINUS();
    antlr4::tree::TerminalNode* MINUS(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExpressionContext* expression();

  class  TermContext : public antlr4::ParserRuleContext {
  public:
    TermContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<FactorContext *> factor();
    FactorContext* factor(size_t i);
    std::vector<antlr4::tree::TerminalNode *> STAR();
    antlr4::tree::TerminalNode* STAR(size_t i);
    std::vector<antlr4::tree::TerminalNode *> DIVIDE();
    antlr4::tree::TerminalNode* DIVIDE(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  TermContext* term();

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
    antlr4::tree::TerminalNode *LR_BRACKET();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *RR_BRACKET();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  FactorContext* factor();

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
    std::vector<antlr4::tree::TerminalNode *> RR_BRACKET();
    antlr4::tree::TerminalNode* RR_BRACKET(size_t i);
    std::vector<antlr4::tree::TerminalNode *> DOT();
    antlr4::tree::TerminalNode* DOT(size_t i);
    std::vector<AgregatorContext *> agregator();
    AgregatorContext* agregator(size_t i);
    std::vector<antlr4::tree::TerminalNode *> PLUS();
    antlr4::tree::TerminalNode* PLUS(size_t i);
    std::vector<antlr4::tree::TerminalNode *> MINUS();
    antlr4::tree::TerminalNode* MINUS(size_t i);


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

