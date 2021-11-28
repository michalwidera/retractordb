
    #include <iostream>


// Generated from RQL.g4 by ANTLR 4.9.3


#include "RQLVisitor.h"

#include "RQLParser.h"


using namespace antlrcpp;
using namespace antlr4;

RQLParser::RQLParser(TokenStream *input) : Parser(input) {
  _interpreter = new atn::ParserATNSimulator(this, _atn, _decisionToDFA, _sharedContextCache);
}

RQLParser::~RQLParser() {
  delete _interpreter;
}

std::string RQLParser::getGrammarFileName() const {
  return "RQL.g4";
}

const std::vector<std::string>& RQLParser::getRuleNames() const {
  return _ruleNames;
}

dfa::Vocabulary& RQLParser::getVocabulary() const {
  return _vocabulary;
}


//----------------- ProgContext ------------------------------------------------------------------

RQLParser::ProgContext::ProgContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::ProgContext::EOF() {
  return getToken(RQLParser::EOF, 0);
}

std::vector<RQLParser::Select_statementContext *> RQLParser::ProgContext::select_statement() {
  return getRuleContexts<RQLParser::Select_statementContext>();
}

RQLParser::Select_statementContext* RQLParser::ProgContext::select_statement(size_t i) {
  return getRuleContext<RQLParser::Select_statementContext>(i);
}

std::vector<RQLParser::Declare_statementContext *> RQLParser::ProgContext::declare_statement() {
  return getRuleContexts<RQLParser::Declare_statementContext>();
}

RQLParser::Declare_statementContext* RQLParser::ProgContext::declare_statement(size_t i) {
  return getRuleContext<RQLParser::Declare_statementContext>(i);
}


size_t RQLParser::ProgContext::getRuleIndex() const {
  return RQLParser::RuleProg;
}


antlrcpp::Any RQLParser::ProgContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitProg(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::ProgContext* RQLParser::prog() {
  ProgContext *_localctx = _tracker.createInstance<ProgContext>(_ctx, getState());
  enterRule(_localctx, 0, RQLParser::RuleProg);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(36); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(36);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::SELECT: {
          setState(34);
          select_statement();
          break;
        }

        case RQLParser::DECLARE: {
          setState(35);
          declare_statement();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(38); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == RQLParser::SELECT

    || _la == RQLParser::DECLARE);
    setState(40);
    match(RQLParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Select_statementContext ------------------------------------------------------------------

RQLParser::Select_statementContext::Select_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::Select_statementContext::SELECT() {
  return getToken(RQLParser::SELECT, 0);
}

tree::TerminalNode* RQLParser::Select_statementContext::STREAM() {
  return getToken(RQLParser::STREAM, 0);
}

tree::TerminalNode* RQLParser::Select_statementContext::FROM() {
  return getToken(RQLParser::FROM, 0);
}

RQLParser::Select_listContext* RQLParser::Select_statementContext::select_list() {
  return getRuleContext<RQLParser::Select_listContext>(0);
}

tree::TerminalNode* RQLParser::Select_statementContext::ID() {
  return getToken(RQLParser::ID, 0);
}

RQLParser::Stream_expressionContext* RQLParser::Select_statementContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}


size_t RQLParser::Select_statementContext::getRuleIndex() const {
  return RQLParser::RuleSelect_statement;
}


antlrcpp::Any RQLParser::Select_statementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitSelect_statement(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Select_statementContext* RQLParser::select_statement() {
  Select_statementContext *_localctx = _tracker.createInstance<Select_statementContext>(_ctx, getState());
  enterRule(_localctx, 2, RQLParser::RuleSelect_statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(42);
    match(RQLParser::SELECT);
    setState(43);
    antlrcpp::downCast<Select_statementContext *>(_localctx)->columns = select_list();
    setState(44);
    match(RQLParser::STREAM);
    setState(45);
    antlrcpp::downCast<Select_statementContext *>(_localctx)->id = match(RQLParser::ID);
    setState(46);
    match(RQLParser::FROM);
    setState(47);
    antlrcpp::downCast<Select_statementContext *>(_localctx)->from = stream_expression();

                          std::cout << "::columns ID:" << (antlrcpp::downCast<Select_statementContext *>(_localctx)->columns != nullptr ? _input->getText(antlrcpp::downCast<Select_statementContext *>(_localctx)->columns->start, antlrcpp::downCast<Select_statementContext *>(_localctx)->columns->stop) : nullptr) << std::endl;
                          std::cout << "::id ID:" << (antlrcpp::downCast<Select_statementContext *>(_localctx)->id != nullptr ? antlrcpp::downCast<Select_statementContext *>(_localctx)->id->getText() : "") << std::endl;
                          std::cout << "::from ID:" << (antlrcpp::downCast<Select_statementContext *>(_localctx)->from != nullptr ? _input->getText(antlrcpp::downCast<Select_statementContext *>(_localctx)->from->start, antlrcpp::downCast<Select_statementContext *>(_localctx)->from->stop) : nullptr) << std::endl;
                          
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Declare_statementContext ------------------------------------------------------------------

RQLParser::Declare_statementContext::Declare_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::Declare_statementContext::DECLARE() {
  return getToken(RQLParser::DECLARE, 0);
}

RQLParser::Column_name_listContext* RQLParser::Declare_statementContext::column_name_list() {
  return getRuleContext<RQLParser::Column_name_listContext>(0);
}

tree::TerminalNode* RQLParser::Declare_statementContext::STREAM() {
  return getToken(RQLParser::STREAM, 0);
}

tree::TerminalNode* RQLParser::Declare_statementContext::ID() {
  return getToken(RQLParser::ID, 0);
}

tree::TerminalNode* RQLParser::Declare_statementContext::COMMA() {
  return getToken(RQLParser::COMMA, 0);
}

tree::TerminalNode* RQLParser::Declare_statementContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

tree::TerminalNode* RQLParser::Declare_statementContext::FLOAT() {
  return getToken(RQLParser::FLOAT, 0);
}

tree::TerminalNode* RQLParser::Declare_statementContext::RATIONAL() {
  return getToken(RQLParser::RATIONAL, 0);
}

tree::TerminalNode* RQLParser::Declare_statementContext::FILE() {
  return getToken(RQLParser::FILE, 0);
}

tree::TerminalNode* RQLParser::Declare_statementContext::STRING() {
  return getToken(RQLParser::STRING, 0);
}


size_t RQLParser::Declare_statementContext::getRuleIndex() const {
  return RQLParser::RuleDeclare_statement;
}


antlrcpp::Any RQLParser::Declare_statementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitDeclare_statement(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Declare_statementContext* RQLParser::declare_statement() {
  Declare_statementContext *_localctx = _tracker.createInstance<Declare_statementContext>(_ctx, getState());
  enterRule(_localctx, 4, RQLParser::RuleDeclare_statement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(50);
    match(RQLParser::DECLARE);
    setState(51);
    column_name_list();
    setState(52);
    match(RQLParser::STREAM);
    setState(53);
    match(RQLParser::ID);
    setState(54);
    match(RQLParser::COMMA);
    setState(55);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << RQLParser::FLOAT)
      | (1ULL << RQLParser::DECIMAL)
      | (1ULL << RQLParser::RATIONAL))) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(58);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::FILE) {
      setState(56);
      match(RQLParser::FILE);
      setState(57);
      match(RQLParser::STRING);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Column_name_listContext ------------------------------------------------------------------

RQLParser::Column_name_listContext::Column_name_listContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<RQLParser::Column_typeContext *> RQLParser::Column_name_listContext::column_type() {
  return getRuleContexts<RQLParser::Column_typeContext>();
}

RQLParser::Column_typeContext* RQLParser::Column_name_listContext::column_type(size_t i) {
  return getRuleContext<RQLParser::Column_typeContext>(i);
}

std::vector<tree::TerminalNode *> RQLParser::Column_name_listContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::Column_name_listContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

std::vector<tree::TerminalNode *> RQLParser::Column_name_listContext::COMMA() {
  return getTokens(RQLParser::COMMA);
}

tree::TerminalNode* RQLParser::Column_name_listContext::COMMA(size_t i) {
  return getToken(RQLParser::COMMA, i);
}


size_t RQLParser::Column_name_listContext::getRuleIndex() const {
  return RQLParser::RuleColumn_name_list;
}


antlrcpp::Any RQLParser::Column_name_listContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitColumn_name_list(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Column_name_listContext* RQLParser::column_name_list() {
  Column_name_listContext *_localctx = _tracker.createInstance<Column_name_listContext>(_ctx, getState());
  enterRule(_localctx, 6, RQLParser::RuleColumn_name_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(60);
    antlrcpp::downCast<Column_name_listContext *>(_localctx)->idToken = match(RQLParser::ID);
    antlrcpp::downCast<Column_name_listContext *>(_localctx)->column.push_back(antlrcpp::downCast<Column_name_listContext *>(_localctx)->idToken);
    setState(61);
    column_type();
    setState(67);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(62);
      match(RQLParser::COMMA);
      setState(63);
      antlrcpp::downCast<Column_name_listContext *>(_localctx)->idToken = match(RQLParser::ID);
      antlrcpp::downCast<Column_name_listContext *>(_localctx)->column.push_back(antlrcpp::downCast<Column_name_listContext *>(_localctx)->idToken);
      setState(64);
      column_type();
      setState(69);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Column_typeContext ------------------------------------------------------------------

RQLParser::Column_typeContext::Column_typeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::Column_typeContext::FLOAT_T() {
  return getToken(RQLParser::FLOAT_T, 0);
}

tree::TerminalNode* RQLParser::Column_typeContext::BYTE_T() {
  return getToken(RQLParser::BYTE_T, 0);
}

tree::TerminalNode* RQLParser::Column_typeContext::INTEGER_T() {
  return getToken(RQLParser::INTEGER_T, 0);
}

tree::TerminalNode* RQLParser::Column_typeContext::UNSIGNED_T() {
  return getToken(RQLParser::UNSIGNED_T, 0);
}

tree::TerminalNode* RQLParser::Column_typeContext::LS_BRACKET() {
  return getToken(RQLParser::LS_BRACKET, 0);
}

tree::TerminalNode* RQLParser::Column_typeContext::RS_BRACKET() {
  return getToken(RQLParser::RS_BRACKET, 0);
}

tree::TerminalNode* RQLParser::Column_typeContext::STRING_T() {
  return getToken(RQLParser::STRING_T, 0);
}

tree::TerminalNode* RQLParser::Column_typeContext::ARRAY_B_T() {
  return getToken(RQLParser::ARRAY_B_T, 0);
}

tree::TerminalNode* RQLParser::Column_typeContext::ARRAY_I_T() {
  return getToken(RQLParser::ARRAY_I_T, 0);
}

tree::TerminalNode* RQLParser::Column_typeContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}


size_t RQLParser::Column_typeContext::getRuleIndex() const {
  return RQLParser::RuleColumn_type;
}


antlrcpp::Any RQLParser::Column_typeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitColumn_type(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Column_typeContext* RQLParser::column_type() {
  Column_typeContext *_localctx = _tracker.createInstance<Column_typeContext>(_ctx, getState());
  enterRule(_localctx, 8, RQLParser::RuleColumn_type);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(75);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::FLOAT_T:
      case RQLParser::BYTE_T:
      case RQLParser::INTEGER_T:
      case RQLParser::UNSIGNED_T: {
        setState(70);
        _la = _input->LA(1);
        if (!((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & ((1ULL << RQLParser::FLOAT_T)
          | (1ULL << RQLParser::BYTE_T)
          | (1ULL << RQLParser::INTEGER_T)
          | (1ULL << RQLParser::UNSIGNED_T))) != 0))) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        break;
      }

      case RQLParser::STRING_T:
      case RQLParser::ARRAY_B_T:
      case RQLParser::ARRAY_I_T: {
        setState(71);
        _la = _input->LA(1);
        if (!((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & ((1ULL << RQLParser::STRING_T)
          | (1ULL << RQLParser::ARRAY_B_T)
          | (1ULL << RQLParser::ARRAY_I_T))) != 0))) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(72);
        match(RQLParser::LS_BRACKET);
        setState(73);
        antlrcpp::downCast<Column_typeContext *>(_localctx)->type_size = match(RQLParser::DECIMAL);
        setState(74);
        match(RQLParser::RS_BRACKET);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Select_listContext ------------------------------------------------------------------

RQLParser::Select_listContext::Select_listContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<RQLParser::Select_list_elemContext *> RQLParser::Select_listContext::select_list_elem() {
  return getRuleContexts<RQLParser::Select_list_elemContext>();
}

RQLParser::Select_list_elemContext* RQLParser::Select_listContext::select_list_elem(size_t i) {
  return getRuleContext<RQLParser::Select_list_elemContext>(i);
}

std::vector<tree::TerminalNode *> RQLParser::Select_listContext::COMMA() {
  return getTokens(RQLParser::COMMA);
}

tree::TerminalNode* RQLParser::Select_listContext::COMMA(size_t i) {
  return getToken(RQLParser::COMMA, i);
}


size_t RQLParser::Select_listContext::getRuleIndex() const {
  return RQLParser::RuleSelect_list;
}


antlrcpp::Any RQLParser::Select_listContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitSelect_list(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Select_listContext* RQLParser::select_list() {
  Select_listContext *_localctx = _tracker.createInstance<Select_listContext>(_ctx, getState());
  enterRule(_localctx, 10, RQLParser::RuleSelect_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(77);
    antlrcpp::downCast<Select_listContext *>(_localctx)->select_list_elemContext = select_list_elem();
    antlrcpp::downCast<Select_listContext *>(_localctx)->column.push_back(antlrcpp::downCast<Select_listContext *>(_localctx)->select_list_elemContext);
    setState(82);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(78);
      match(RQLParser::COMMA);
      setState(79);
      antlrcpp::downCast<Select_listContext *>(_localctx)->select_list_elemContext = select_list_elem();
      antlrcpp::downCast<Select_listContext *>(_localctx)->column.push_back(antlrcpp::downCast<Select_listContext *>(_localctx)->select_list_elemContext);
      setState(84);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Select_list_elemContext ------------------------------------------------------------------

RQLParser::Select_list_elemContext::Select_list_elemContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

RQLParser::AsteriskContext* RQLParser::Select_list_elemContext::asterisk() {
  return getRuleContext<RQLParser::AsteriskContext>(0);
}

RQLParser::ExpressionContext* RQLParser::Select_list_elemContext::expression() {
  return getRuleContext<RQLParser::ExpressionContext>(0);
}


size_t RQLParser::Select_list_elemContext::getRuleIndex() const {
  return RQLParser::RuleSelect_list_elem;
}


antlrcpp::Any RQLParser::Select_list_elemContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitSelect_list_elem(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Select_list_elemContext* RQLParser::select_list_elem() {
  Select_list_elemContext *_localctx = _tracker.createInstance<Select_list_elemContext>(_ctx, getState());
  enterRule(_localctx, 12, RQLParser::RuleSelect_list_elem);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(87);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 6, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(85);
      asterisk();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(86);
      expression();
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Field_idContext ------------------------------------------------------------------

RQLParser::Field_idContext::Field_idContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> RQLParser::Field_idContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::Field_idContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

tree::TerminalNode* RQLParser::Field_idContext::LS_BRACKET() {
  return getToken(RQLParser::LS_BRACKET, 0);
}

tree::TerminalNode* RQLParser::Field_idContext::UNDERLINE() {
  return getToken(RQLParser::UNDERLINE, 0);
}

tree::TerminalNode* RQLParser::Field_idContext::RS_BRACKET() {
  return getToken(RQLParser::RS_BRACKET, 0);
}

tree::TerminalNode* RQLParser::Field_idContext::DOT() {
  return getToken(RQLParser::DOT, 0);
}

tree::TerminalNode* RQLParser::Field_idContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}


size_t RQLParser::Field_idContext::getRuleIndex() const {
  return RQLParser::RuleField_id;
}


antlrcpp::Any RQLParser::Field_idContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitField_id(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Field_idContext* RQLParser::field_id() {
  Field_idContext *_localctx = _tracker.createInstance<Field_idContext>(_ctx, getState());
  enterRule(_localctx, 14, RQLParser::RuleField_id);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(101);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 7, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(89);
      antlrcpp::downCast<Field_idContext *>(_localctx)->column_name = match(RQLParser::ID);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(90);
      antlrcpp::downCast<Field_idContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(91);
      match(RQLParser::LS_BRACKET);
      setState(92);
      match(RQLParser::UNDERLINE);
      setState(93);
      match(RQLParser::RS_BRACKET);
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(94);
      antlrcpp::downCast<Field_idContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(95);
      match(RQLParser::DOT);
      setState(96);
      antlrcpp::downCast<Field_idContext *>(_localctx)->column_name = match(RQLParser::ID);
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(97);
      antlrcpp::downCast<Field_idContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(98);
      match(RQLParser::LS_BRACKET);
      setState(99);
      antlrcpp::downCast<Field_idContext *>(_localctx)->column_index = match(RQLParser::DECIMAL);
      setState(100);
      match(RQLParser::RS_BRACKET);
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Unary_op_expressionContext ------------------------------------------------------------------

RQLParser::Unary_op_expressionContext::Unary_op_expressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::Unary_op_expressionContext::BIT_NOT() {
  return getToken(RQLParser::BIT_NOT, 0);
}

RQLParser::ExpressionContext* RQLParser::Unary_op_expressionContext::expression() {
  return getRuleContext<RQLParser::ExpressionContext>(0);
}

tree::TerminalNode* RQLParser::Unary_op_expressionContext::PLUS() {
  return getToken(RQLParser::PLUS, 0);
}

tree::TerminalNode* RQLParser::Unary_op_expressionContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}


size_t RQLParser::Unary_op_expressionContext::getRuleIndex() const {
  return RQLParser::RuleUnary_op_expression;
}


antlrcpp::Any RQLParser::Unary_op_expressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitUnary_op_expression(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Unary_op_expressionContext* RQLParser::unary_op_expression() {
  Unary_op_expressionContext *_localctx = _tracker.createInstance<Unary_op_expressionContext>(_ctx, getState());
  enterRule(_localctx, 16, RQLParser::RuleUnary_op_expression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(107);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::BIT_NOT: {
        enterOuterAlt(_localctx, 1);
        setState(103);
        match(RQLParser::BIT_NOT);
        setState(104);
        expression();
        break;
      }

      case RQLParser::PLUS:
      case RQLParser::MINUS: {
        enterOuterAlt(_localctx, 2);
        setState(105);
        antlrcpp::downCast<Unary_op_expressionContext *>(_localctx)->op = _input->LT(1);
        _la = _input->LA(1);
        if (!(_la == RQLParser::PLUS

        || _la == RQLParser::MINUS)) {
          antlrcpp::downCast<Unary_op_expressionContext *>(_localctx)->op = _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(106);
        expression();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AsteriskContext ------------------------------------------------------------------

RQLParser::AsteriskContext::AsteriskContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::AsteriskContext::STAR() {
  return getToken(RQLParser::STAR, 0);
}

tree::TerminalNode* RQLParser::AsteriskContext::ID() {
  return getToken(RQLParser::ID, 0);
}

tree::TerminalNode* RQLParser::AsteriskContext::DOT() {
  return getToken(RQLParser::DOT, 0);
}


size_t RQLParser::AsteriskContext::getRuleIndex() const {
  return RQLParser::RuleAsterisk;
}


antlrcpp::Any RQLParser::AsteriskContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitAsterisk(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::AsteriskContext* RQLParser::asterisk() {
  AsteriskContext *_localctx = _tracker.createInstance<AsteriskContext>(_ctx, getState());
  enterRule(_localctx, 18, RQLParser::RuleAsterisk);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(111);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::ID) {
      setState(109);
      match(RQLParser::ID);
      setState(110);
      match(RQLParser::DOT);
    }
    setState(113);
    match(RQLParser::STAR);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpressionContext ------------------------------------------------------------------

RQLParser::ExpressionContext::ExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<RQLParser::TermContext *> RQLParser::ExpressionContext::term() {
  return getRuleContexts<RQLParser::TermContext>();
}

RQLParser::TermContext* RQLParser::ExpressionContext::term(size_t i) {
  return getRuleContext<RQLParser::TermContext>(i);
}

std::vector<tree::TerminalNode *> RQLParser::ExpressionContext::PLUS() {
  return getTokens(RQLParser::PLUS);
}

tree::TerminalNode* RQLParser::ExpressionContext::PLUS(size_t i) {
  return getToken(RQLParser::PLUS, i);
}

std::vector<tree::TerminalNode *> RQLParser::ExpressionContext::MINUS() {
  return getTokens(RQLParser::MINUS);
}

tree::TerminalNode* RQLParser::ExpressionContext::MINUS(size_t i) {
  return getToken(RQLParser::MINUS, i);
}


size_t RQLParser::ExpressionContext::getRuleIndex() const {
  return RQLParser::RuleExpression;
}


antlrcpp::Any RQLParser::ExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitExpression(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::ExpressionContext* RQLParser::expression() {
  ExpressionContext *_localctx = _tracker.createInstance<ExpressionContext>(_ctx, getState());
  enterRule(_localctx, 20, RQLParser::RuleExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(115);
    term();
    setState(122);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 11, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(120);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
          case RQLParser::PLUS: {
            setState(116);
            match(RQLParser::PLUS);
            setState(117);
            term();
            break;
          }

          case RQLParser::MINUS: {
            setState(118);
            match(RQLParser::MINUS);
            setState(119);
            term();
            break;
          }

        default:
          throw NoViableAltException(this);
        } 
      }
      setState(124);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 11, _ctx);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- TermContext ------------------------------------------------------------------

RQLParser::TermContext::TermContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<RQLParser::FactorContext *> RQLParser::TermContext::factor() {
  return getRuleContexts<RQLParser::FactorContext>();
}

RQLParser::FactorContext* RQLParser::TermContext::factor(size_t i) {
  return getRuleContext<RQLParser::FactorContext>(i);
}

std::vector<tree::TerminalNode *> RQLParser::TermContext::STAR() {
  return getTokens(RQLParser::STAR);
}

tree::TerminalNode* RQLParser::TermContext::STAR(size_t i) {
  return getToken(RQLParser::STAR, i);
}

std::vector<tree::TerminalNode *> RQLParser::TermContext::DIVIDE() {
  return getTokens(RQLParser::DIVIDE);
}

tree::TerminalNode* RQLParser::TermContext::DIVIDE(size_t i) {
  return getToken(RQLParser::DIVIDE, i);
}


size_t RQLParser::TermContext::getRuleIndex() const {
  return RQLParser::RuleTerm;
}


antlrcpp::Any RQLParser::TermContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitTerm(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::TermContext* RQLParser::term() {
  TermContext *_localctx = _tracker.createInstance<TermContext>(_ctx, getState());
  enterRule(_localctx, 22, RQLParser::RuleTerm);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(125);
    factor();
    setState(132);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 13, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(130);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
          case RQLParser::STAR: {
            setState(126);
            match(RQLParser::STAR);
            setState(127);
            factor();
            break;
          }

          case RQLParser::DIVIDE: {
            setState(128);
            match(RQLParser::DIVIDE);
            setState(129);
            factor();
            break;
          }

        default:
          throw NoViableAltException(this);
        } 
      }
      setState(134);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 13, _ctx);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FactorContext ------------------------------------------------------------------

RQLParser::FactorContext::FactorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::FactorContext::FLOAT() {
  return getToken(RQLParser::FLOAT, 0);
}

tree::TerminalNode* RQLParser::FactorContext::REAL() {
  return getToken(RQLParser::REAL, 0);
}

tree::TerminalNode* RQLParser::FactorContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

RQLParser::Unary_op_expressionContext* RQLParser::FactorContext::unary_op_expression() {
  return getRuleContext<RQLParser::Unary_op_expressionContext>(0);
}

RQLParser::Function_callContext* RQLParser::FactorContext::function_call() {
  return getRuleContext<RQLParser::Function_callContext>(0);
}

RQLParser::Field_idContext* RQLParser::FactorContext::field_id() {
  return getRuleContext<RQLParser::Field_idContext>(0);
}

RQLParser::AgregatorContext* RQLParser::FactorContext::agregator() {
  return getRuleContext<RQLParser::AgregatorContext>(0);
}

tree::TerminalNode* RQLParser::FactorContext::LR_BRACKET() {
  return getToken(RQLParser::LR_BRACKET, 0);
}

RQLParser::ExpressionContext* RQLParser::FactorContext::expression() {
  return getRuleContext<RQLParser::ExpressionContext>(0);
}

tree::TerminalNode* RQLParser::FactorContext::RR_BRACKET() {
  return getToken(RQLParser::RR_BRACKET, 0);
}


size_t RQLParser::FactorContext::getRuleIndex() const {
  return RQLParser::RuleFactor;
}


antlrcpp::Any RQLParser::FactorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitFactor(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::FactorContext* RQLParser::factor() {
  FactorContext *_localctx = _tracker.createInstance<FactorContext>(_ctx, getState());
  enterRule(_localctx, 24, RQLParser::RuleFactor);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(146);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::FLOAT: {
        enterOuterAlt(_localctx, 1);
        setState(135);
        match(RQLParser::FLOAT);
        break;
      }

      case RQLParser::REAL: {
        enterOuterAlt(_localctx, 2);
        setState(136);
        match(RQLParser::REAL);
        break;
      }

      case RQLParser::DECIMAL: {
        enterOuterAlt(_localctx, 3);
        setState(137);
        match(RQLParser::DECIMAL);
        break;
      }

      case RQLParser::PLUS:
      case RQLParser::MINUS:
      case RQLParser::BIT_NOT: {
        enterOuterAlt(_localctx, 4);
        setState(138);
        unary_op_expression();
        break;
      }

      case RQLParser::SQRT:
      case RQLParser::CEIL:
      case RQLParser::ABS:
      case RQLParser::FLOOR:
      case RQLParser::SIGN:
      case RQLParser::CHR:
      case RQLParser::LENGTH:
      case RQLParser::TO_NUMBER:
      case RQLParser::TO_TIMESTAMP:
      case RQLParser::FLOAT_CAST:
      case RQLParser::INT_CAST:
      case RQLParser::COUNT:
      case RQLParser::CRC:
      case RQLParser::SUM:
      case RQLParser::ISZERO:
      case RQLParser::ISNONZERO: {
        enterOuterAlt(_localctx, 5);
        setState(139);
        function_call();
        break;
      }

      case RQLParser::ID: {
        enterOuterAlt(_localctx, 6);
        setState(140);
        field_id();
        break;
      }

      case RQLParser::MIN:
      case RQLParser::MAX:
      case RQLParser::AVG:
      case RQLParser::SUMC: {
        enterOuterAlt(_localctx, 7);
        setState(141);
        agregator();
        break;
      }

      case RQLParser::LR_BRACKET: {
        enterOuterAlt(_localctx, 8);
        setState(142);
        match(RQLParser::LR_BRACKET);
        setState(143);
        expression();
        setState(144);
        match(RQLParser::RR_BRACKET);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Stream_expressionContext ------------------------------------------------------------------

RQLParser::Stream_expressionContext::Stream_expressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<RQLParser::Stream_termContext *> RQLParser::Stream_expressionContext::stream_term() {
  return getRuleContexts<RQLParser::Stream_termContext>();
}

RQLParser::Stream_termContext* RQLParser::Stream_expressionContext::stream_term(size_t i) {
  return getRuleContext<RQLParser::Stream_termContext>(i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_expressionContext::GREATER() {
  return getTokens(RQLParser::GREATER);
}

tree::TerminalNode* RQLParser::Stream_expressionContext::GREATER(size_t i) {
  return getToken(RQLParser::GREATER, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_expressionContext::DECIMAL() {
  return getTokens(RQLParser::DECIMAL);
}

tree::TerminalNode* RQLParser::Stream_expressionContext::DECIMAL(size_t i) {
  return getToken(RQLParser::DECIMAL, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_expressionContext::PLUS() {
  return getTokens(RQLParser::PLUS);
}

tree::TerminalNode* RQLParser::Stream_expressionContext::PLUS(size_t i) {
  return getToken(RQLParser::PLUS, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_expressionContext::MINUS() {
  return getTokens(RQLParser::MINUS);
}

tree::TerminalNode* RQLParser::Stream_expressionContext::MINUS(size_t i) {
  return getToken(RQLParser::MINUS, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_expressionContext::RATIONAL() {
  return getTokens(RQLParser::RATIONAL);
}

tree::TerminalNode* RQLParser::Stream_expressionContext::RATIONAL(size_t i) {
  return getToken(RQLParser::RATIONAL, i);
}


size_t RQLParser::Stream_expressionContext::getRuleIndex() const {
  return RQLParser::RuleStream_expression;
}


antlrcpp::Any RQLParser::Stream_expressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitStream_expression(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Stream_expressionContext* RQLParser::stream_expression() {
  Stream_expressionContext *_localctx = _tracker.createInstance<Stream_expressionContext>(_ctx, getState());
  enterRule(_localctx, 26, RQLParser::RuleStream_expression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(148);
    stream_term();
    setState(157);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (((((_la - 40) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 40)) & ((1ULL << (RQLParser::GREATER - 40))
      | (1ULL << (RQLParser::PLUS - 40))
      | (1ULL << (RQLParser::MINUS - 40)))) != 0)) {
      setState(155);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::GREATER: {
          setState(149);
          match(RQLParser::GREATER);
          setState(150);
          match(RQLParser::DECIMAL);
          break;
        }

        case RQLParser::PLUS: {
          setState(151);
          match(RQLParser::PLUS);
          setState(152);
          stream_term();
          break;
        }

        case RQLParser::MINUS: {
          setState(153);
          match(RQLParser::MINUS);
          setState(154);
          match(RQLParser::RATIONAL);
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(159);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Stream_termContext ------------------------------------------------------------------

RQLParser::Stream_termContext::Stream_termContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::Stream_termContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::SHARP() {
  return getTokens(RQLParser::SHARP);
}

tree::TerminalNode* RQLParser::Stream_termContext::SHARP(size_t i) {
  return getToken(RQLParser::SHARP, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::AND() {
  return getTokens(RQLParser::AND);
}

tree::TerminalNode* RQLParser::Stream_termContext::AND(size_t i) {
  return getToken(RQLParser::AND, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::RATIONAL() {
  return getTokens(RQLParser::RATIONAL);
}

tree::TerminalNode* RQLParser::Stream_termContext::RATIONAL(size_t i) {
  return getToken(RQLParser::RATIONAL, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::MOD() {
  return getTokens(RQLParser::MOD);
}

tree::TerminalNode* RQLParser::Stream_termContext::MOD(size_t i) {
  return getToken(RQLParser::MOD, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::AT() {
  return getTokens(RQLParser::AT);
}

tree::TerminalNode* RQLParser::Stream_termContext::AT(size_t i) {
  return getToken(RQLParser::AT, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::LR_BRACKET() {
  return getTokens(RQLParser::LR_BRACKET);
}

tree::TerminalNode* RQLParser::Stream_termContext::LR_BRACKET(size_t i) {
  return getToken(RQLParser::LR_BRACKET, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::DECIMAL() {
  return getTokens(RQLParser::DECIMAL);
}

tree::TerminalNode* RQLParser::Stream_termContext::DECIMAL(size_t i) {
  return getToken(RQLParser::DECIMAL, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::COMMA() {
  return getTokens(RQLParser::COMMA);
}

tree::TerminalNode* RQLParser::Stream_termContext::COMMA(size_t i) {
  return getToken(RQLParser::COMMA, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::RR_BRACKET() {
  return getTokens(RQLParser::RR_BRACKET);
}

tree::TerminalNode* RQLParser::Stream_termContext::RR_BRACKET(size_t i) {
  return getToken(RQLParser::RR_BRACKET, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::DOT() {
  return getTokens(RQLParser::DOT);
}

tree::TerminalNode* RQLParser::Stream_termContext::DOT(size_t i) {
  return getToken(RQLParser::DOT, i);
}

std::vector<RQLParser::AgregatorContext *> RQLParser::Stream_termContext::agregator() {
  return getRuleContexts<RQLParser::AgregatorContext>();
}

RQLParser::AgregatorContext* RQLParser::Stream_termContext::agregator(size_t i) {
  return getRuleContext<RQLParser::AgregatorContext>(i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::PLUS() {
  return getTokens(RQLParser::PLUS);
}

tree::TerminalNode* RQLParser::Stream_termContext::PLUS(size_t i) {
  return getToken(RQLParser::PLUS, i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::MINUS() {
  return getTokens(RQLParser::MINUS);
}

tree::TerminalNode* RQLParser::Stream_termContext::MINUS(size_t i) {
  return getToken(RQLParser::MINUS, i);
}


size_t RQLParser::Stream_termContext::getRuleIndex() const {
  return RQLParser::RuleStream_term;
}


antlrcpp::Any RQLParser::Stream_termContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitStream_term(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Stream_termContext* RQLParser::stream_term() {
  Stream_termContext *_localctx = _tracker.createInstance<Stream_termContext>(_ctx, getState());
  enterRule(_localctx, 28, RQLParser::RuleStream_term);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(160);
    match(RQLParser::ID);
    setState(180);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << RQLParser::DOT)
      | (1ULL << RQLParser::AT)
      | (1ULL << RQLParser::SHARP)
      | (1ULL << RQLParser::AND)
      | (1ULL << RQLParser::MOD))) != 0)) {
      setState(178);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::SHARP: {
          setState(161);
          match(RQLParser::SHARP);
          setState(162);
          match(RQLParser::ID);
          break;
        }

        case RQLParser::AND: {
          setState(163);
          match(RQLParser::AND);
          setState(164);
          match(RQLParser::RATIONAL);
          break;
        }

        case RQLParser::MOD: {
          setState(165);
          match(RQLParser::MOD);
          setState(166);
          match(RQLParser::RATIONAL);
          break;
        }

        case RQLParser::AT: {
          setState(167);
          match(RQLParser::AT);
          setState(168);
          match(RQLParser::LR_BRACKET);
          setState(169);
          match(RQLParser::DECIMAL);
          setState(170);
          match(RQLParser::COMMA);
          setState(172);
          _errHandler->sync(this);

          _la = _input->LA(1);
          if (_la == RQLParser::PLUS

          || _la == RQLParser::MINUS) {
            setState(171);
            _la = _input->LA(1);
            if (!(_la == RQLParser::PLUS

            || _la == RQLParser::MINUS)) {
            _errHandler->recoverInline(this);
            }
            else {
              _errHandler->reportMatch(this);
              consume();
            }
          }
          setState(174);
          match(RQLParser::DECIMAL);
          setState(175);
          match(RQLParser::RR_BRACKET);
          break;
        }

        case RQLParser::DOT: {
          setState(176);
          match(RQLParser::DOT);
          setState(177);
          agregator();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(182);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AgregatorContext ------------------------------------------------------------------

RQLParser::AgregatorContext::AgregatorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::AgregatorContext::MIN() {
  return getToken(RQLParser::MIN, 0);
}

tree::TerminalNode* RQLParser::AgregatorContext::MAX() {
  return getToken(RQLParser::MAX, 0);
}

tree::TerminalNode* RQLParser::AgregatorContext::AVG() {
  return getToken(RQLParser::AVG, 0);
}

tree::TerminalNode* RQLParser::AgregatorContext::SUMC() {
  return getToken(RQLParser::SUMC, 0);
}


size_t RQLParser::AgregatorContext::getRuleIndex() const {
  return RQLParser::RuleAgregator;
}


antlrcpp::Any RQLParser::AgregatorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitAgregator(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::AgregatorContext* RQLParser::agregator() {
  AgregatorContext *_localctx = _tracker.createInstance<AgregatorContext>(_ctx, getState());
  enterRule(_localctx, 30, RQLParser::RuleAgregator);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(183);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << RQLParser::MIN)
      | (1ULL << RQLParser::MAX)
      | (1ULL << RQLParser::AVG)
      | (1ULL << RQLParser::SUMC))) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Function_callContext ------------------------------------------------------------------

RQLParser::Function_callContext::Function_callContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::Function_callContext::LR_BRACKET() {
  return getToken(RQLParser::LR_BRACKET, 0);
}

RQLParser::ExpressionContext* RQLParser::Function_callContext::expression() {
  return getRuleContext<RQLParser::ExpressionContext>(0);
}

tree::TerminalNode* RQLParser::Function_callContext::RR_BRACKET() {
  return getToken(RQLParser::RR_BRACKET, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::SQRT() {
  return getToken(RQLParser::SQRT, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::CEIL() {
  return getToken(RQLParser::CEIL, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::ABS() {
  return getToken(RQLParser::ABS, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::FLOOR() {
  return getToken(RQLParser::FLOOR, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::SIGN() {
  return getToken(RQLParser::SIGN, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::CHR() {
  return getToken(RQLParser::CHR, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::LENGTH() {
  return getToken(RQLParser::LENGTH, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::TO_NUMBER() {
  return getToken(RQLParser::TO_NUMBER, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::TO_TIMESTAMP() {
  return getToken(RQLParser::TO_TIMESTAMP, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::FLOAT_CAST() {
  return getToken(RQLParser::FLOAT_CAST, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::INT_CAST() {
  return getToken(RQLParser::INT_CAST, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::COUNT() {
  return getToken(RQLParser::COUNT, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::CRC() {
  return getToken(RQLParser::CRC, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::SUM() {
  return getToken(RQLParser::SUM, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::ISZERO() {
  return getToken(RQLParser::ISZERO, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::ISNONZERO() {
  return getToken(RQLParser::ISNONZERO, 0);
}


size_t RQLParser::Function_callContext::getRuleIndex() const {
  return RQLParser::RuleFunction_call;
}


antlrcpp::Any RQLParser::Function_callContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<RQLVisitor*>(visitor))
    return parserVisitor->visitFunction_call(this);
  else
    return visitor->visitChildren(this);
}

RQLParser::Function_callContext* RQLParser::function_call() {
  Function_callContext *_localctx = _tracker.createInstance<Function_callContext>(_ctx, getState());
  enterRule(_localctx, 32, RQLParser::RuleFunction_call);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(185);
    antlrcpp::downCast<Function_callContext *>(_localctx)->function = _input->LT(1);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << RQLParser::SQRT)
      | (1ULL << RQLParser::CEIL)
      | (1ULL << RQLParser::ABS)
      | (1ULL << RQLParser::FLOOR)
      | (1ULL << RQLParser::SIGN)
      | (1ULL << RQLParser::CHR)
      | (1ULL << RQLParser::LENGTH)
      | (1ULL << RQLParser::TO_NUMBER)
      | (1ULL << RQLParser::TO_TIMESTAMP)
      | (1ULL << RQLParser::FLOAT_CAST)
      | (1ULL << RQLParser::INT_CAST)
      | (1ULL << RQLParser::COUNT)
      | (1ULL << RQLParser::CRC)
      | (1ULL << RQLParser::SUM)
      | (1ULL << RQLParser::ISZERO)
      | (1ULL << RQLParser::ISNONZERO))) != 0))) {
      antlrcpp::downCast<Function_callContext *>(_localctx)->function = _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(186);
    match(RQLParser::LR_BRACKET);
    setState(187);
    expression();
    setState(188);
    match(RQLParser::RR_BRACKET);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

// Static vars and initialization.
std::vector<dfa::DFA> RQLParser::_decisionToDFA;
atn::PredictionContextCache RQLParser::_sharedContextCache;

// We own the ATN which in turn owns the ATN states.
atn::ATN RQLParser::_atn;
std::vector<uint16_t> RQLParser::_serializedATN;

std::vector<std::string> RQLParser::_ruleNames = {
  "prog", "select_statement", "declare_statement", "column_name_list", "column_type", 
  "select_list", "select_list_elem", "field_id", "unary_op_expression", 
  "asterisk", "expression", "term", "factor", "stream_expression", "stream_term", 
  "agregator", "function_call"
};

std::vector<std::string> RQLParser::_literalNames = {
  "", "'FLOAT'", "'STRING'", "'BYTE'", "'INT'", "'UNSIGNED'", "'ARRAY_BYTE'", 
  "'ARRAY_INTEGER'", "'SELECT'", "'STREAM'", "'FROM'", "'DECLARE'", "'FILE'", 
  "'MIN'", "'MAX'", "'AVG'", "'SUMC'", "'SQRT'", "'CEIL'", "'ABS'", "'FLOOR'", 
  "'SIGN'", "'CHR'", "'LENGTH'", "'TO_NUMBER'", "'TO_TIMESTAMP'", "'FLOATC'", 
  "'INTC'", "'COUNT'", "'CRC'", "'SUM'", "'ISZERO'", "'ISNONZERO'", "", 
  "", "", "", "", "", "'='", "'>'", "'<'", "'!'", "'||'", "'.'", "'_'", 
  "'@'", "'#'", "'&'", "'%'", "'$'", "'('", "')'", "'['", "']'", "'{'", 
  "'}'", "','", "';'", "':'", "'::'", "'*'", "'/'", "'+'", "'-'", "'~'", 
  "'|'", "'^'"
};

std::vector<std::string> RQLParser::_symbolicNames = {
  "", "FLOAT_T", "STRING_T", "BYTE_T", "INTEGER_T", "UNSIGNED_T", "ARRAY_B_T", 
  "ARRAY_I_T", "SELECT", "STREAM", "FROM", "DECLARE", "FILE", "MIN", "MAX", 
  "AVG", "SUMC", "SQRT", "CEIL", "ABS", "FLOOR", "SIGN", "CHR", "LENGTH", 
  "TO_NUMBER", "TO_TIMESTAMP", "FLOAT_CAST", "INT_CAST", "COUNT", "CRC", 
  "SUM", "ISZERO", "ISNONZERO", "ID", "STRING", "FLOAT", "DECIMAL", "RATIONAL", 
  "REAL", "EQUAL", "GREATER", "LESS", "EXCLAMATION", "DOUBLE_BAR", "DOT", 
  "UNDERLINE", "AT", "SHARP", "AND", "MOD", "DOLLAR", "LR_BRACKET", "RR_BRACKET", 
  "LS_BRACKET", "RS_BRACKET", "LC_BRACKET", "RC_BRACKET", "COMMA", "SEMI", 
  "COLON", "DOUBLE_COLON", "STAR", "DIVIDE", "PLUS", "MINUS", "BIT_NOT", 
  "BIT_OR", "BIT_XOR", "SPACE", "COMMENT", "LINE_COMMENT"
};

dfa::Vocabulary RQLParser::_vocabulary(_literalNames, _symbolicNames);

std::vector<std::string> RQLParser::_tokenNames;

RQLParser::Initializer::Initializer() {
	for (size_t i = 0; i < _symbolicNames.size(); ++i) {
		std::string name = _vocabulary.getLiteralName(i);
		if (name.empty()) {
			name = _vocabulary.getSymbolicName(i);
		}

		if (name.empty()) {
			_tokenNames.push_back("<INVALID>");
		} else {
      _tokenNames.push_back(name);
    }
	}

  static const uint16_t serializedATNSegment0[] = {
    0x3, 0x608b, 0xa72a, 0x8133, 0xb9ed, 0x417c, 0x3be7, 0x7786, 0x5964, 
       0x3, 0x48, 0xc1, 0x4, 0x2, 0x9, 0x2, 0x4, 0x3, 0x9, 0x3, 0x4, 0x4, 
       0x9, 0x4, 0x4, 0x5, 0x9, 0x5, 0x4, 0x6, 0x9, 0x6, 0x4, 0x7, 0x9, 
       0x7, 0x4, 0x8, 0x9, 0x8, 0x4, 0x9, 0x9, 0x9, 0x4, 0xa, 0x9, 0xa, 
       0x4, 0xb, 0x9, 0xb, 0x4, 0xc, 0x9, 0xc, 0x4, 0xd, 0x9, 0xd, 0x4, 
       0xe, 0x9, 0xe, 0x4, 0xf, 0x9, 0xf, 0x4, 0x10, 0x9, 0x10, 0x4, 0x11, 
       0x9, 0x11, 0x4, 0x12, 0x9, 0x12, 0x3, 0x2, 0x3, 0x2, 0x6, 0x2, 0x27, 
       0xa, 0x2, 0xd, 0x2, 0xe, 0x2, 0x28, 0x3, 0x2, 0x3, 0x2, 0x3, 0x3, 
       0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 
       0x3, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 
       0x3, 0x4, 0x3, 0x4, 0x5, 0x4, 0x3d, 0xa, 0x4, 0x3, 0x5, 0x3, 0x5, 
       0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x7, 0x5, 0x44, 0xa, 0x5, 0xc, 0x5, 
       0xe, 0x5, 0x47, 0xb, 0x5, 0x3, 0x6, 0x3, 0x6, 0x3, 0x6, 0x3, 0x6, 
       0x3, 0x6, 0x5, 0x6, 0x4e, 0xa, 0x6, 0x3, 0x7, 0x3, 0x7, 0x3, 0x7, 
       0x7, 0x7, 0x53, 0xa, 0x7, 0xc, 0x7, 0xe, 0x7, 0x56, 0xb, 0x7, 0x3, 
       0x8, 0x3, 0x8, 0x5, 0x8, 0x5a, 0xa, 0x8, 0x3, 0x9, 0x3, 0x9, 0x3, 
       0x9, 0x3, 0x9, 0x3, 0x9, 0x3, 0x9, 0x3, 0x9, 0x3, 0x9, 0x3, 0x9, 
       0x3, 0x9, 0x3, 0x9, 0x3, 0x9, 0x5, 0x9, 0x68, 0xa, 0x9, 0x3, 0xa, 
       0x3, 0xa, 0x3, 0xa, 0x3, 0xa, 0x5, 0xa, 0x6e, 0xa, 0xa, 0x3, 0xb, 
       0x3, 0xb, 0x5, 0xb, 0x72, 0xa, 0xb, 0x3, 0xb, 0x3, 0xb, 0x3, 0xc, 
       0x3, 0xc, 0x3, 0xc, 0x3, 0xc, 0x3, 0xc, 0x7, 0xc, 0x7b, 0xa, 0xc, 
       0xc, 0xc, 0xe, 0xc, 0x7e, 0xb, 0xc, 0x3, 0xd, 0x3, 0xd, 0x3, 0xd, 
       0x3, 0xd, 0x3, 0xd, 0x7, 0xd, 0x85, 0xa, 0xd, 0xc, 0xd, 0xe, 0xd, 
       0x88, 0xb, 0xd, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 
       0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x5, 
       0xe, 0x95, 0xa, 0xe, 0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 0x3, 
       0xf, 0x3, 0xf, 0x3, 0xf, 0x7, 0xf, 0x9e, 0xa, 0xf, 0xc, 0xf, 0xe, 
       0xf, 0xa1, 0xb, 0xf, 0x3, 0x10, 0x3, 0x10, 0x3, 0x10, 0x3, 0x10, 
       0x3, 0x10, 0x3, 0x10, 0x3, 0x10, 0x3, 0x10, 0x3, 0x10, 0x3, 0x10, 
       0x3, 0x10, 0x3, 0x10, 0x5, 0x10, 0xaf, 0xa, 0x10, 0x3, 0x10, 0x3, 
       0x10, 0x3, 0x10, 0x3, 0x10, 0x7, 0x10, 0xb5, 0xa, 0x10, 0xc, 0x10, 
       0xe, 0x10, 0xb8, 0xb, 0x10, 0x3, 0x11, 0x3, 0x11, 0x3, 0x12, 0x3, 
       0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x2, 0x2, 0x13, 
       0x2, 0x4, 0x6, 0x8, 0xa, 0xc, 0xe, 0x10, 0x12, 0x14, 0x16, 0x18, 
       0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x2, 0x8, 0x3, 0x2, 0x25, 0x27, 0x4, 
       0x2, 0x3, 0x3, 0x5, 0x7, 0x4, 0x2, 0x4, 0x4, 0x8, 0x9, 0x3, 0x2, 
       0x41, 0x42, 0x3, 0x2, 0xf, 0x12, 0x3, 0x2, 0x13, 0x22, 0x2, 0xcf, 
       0x2, 0x26, 0x3, 0x2, 0x2, 0x2, 0x4, 0x2c, 0x3, 0x2, 0x2, 0x2, 0x6, 
       0x34, 0x3, 0x2, 0x2, 0x2, 0x8, 0x3e, 0x3, 0x2, 0x2, 0x2, 0xa, 0x4d, 
       0x3, 0x2, 0x2, 0x2, 0xc, 0x4f, 0x3, 0x2, 0x2, 0x2, 0xe, 0x59, 0x3, 
       0x2, 0x2, 0x2, 0x10, 0x67, 0x3, 0x2, 0x2, 0x2, 0x12, 0x6d, 0x3, 0x2, 
       0x2, 0x2, 0x14, 0x71, 0x3, 0x2, 0x2, 0x2, 0x16, 0x75, 0x3, 0x2, 0x2, 
       0x2, 0x18, 0x7f, 0x3, 0x2, 0x2, 0x2, 0x1a, 0x94, 0x3, 0x2, 0x2, 0x2, 
       0x1c, 0x96, 0x3, 0x2, 0x2, 0x2, 0x1e, 0xa2, 0x3, 0x2, 0x2, 0x2, 0x20, 
       0xb9, 0x3, 0x2, 0x2, 0x2, 0x22, 0xbb, 0x3, 0x2, 0x2, 0x2, 0x24, 0x27, 
       0x5, 0x4, 0x3, 0x2, 0x25, 0x27, 0x5, 0x6, 0x4, 0x2, 0x26, 0x24, 0x3, 
       0x2, 0x2, 0x2, 0x26, 0x25, 0x3, 0x2, 0x2, 0x2, 0x27, 0x28, 0x3, 0x2, 
       0x2, 0x2, 0x28, 0x26, 0x3, 0x2, 0x2, 0x2, 0x28, 0x29, 0x3, 0x2, 0x2, 
       0x2, 0x29, 0x2a, 0x3, 0x2, 0x2, 0x2, 0x2a, 0x2b, 0x7, 0x2, 0x2, 0x3, 
       0x2b, 0x3, 0x3, 0x2, 0x2, 0x2, 0x2c, 0x2d, 0x7, 0xa, 0x2, 0x2, 0x2d, 
       0x2e, 0x5, 0xc, 0x7, 0x2, 0x2e, 0x2f, 0x7, 0xb, 0x2, 0x2, 0x2f, 0x30, 
       0x7, 0x23, 0x2, 0x2, 0x30, 0x31, 0x7, 0xc, 0x2, 0x2, 0x31, 0x32, 
       0x5, 0x1c, 0xf, 0x2, 0x32, 0x33, 0x8, 0x3, 0x1, 0x2, 0x33, 0x5, 0x3, 
       0x2, 0x2, 0x2, 0x34, 0x35, 0x7, 0xd, 0x2, 0x2, 0x35, 0x36, 0x5, 0x8, 
       0x5, 0x2, 0x36, 0x37, 0x7, 0xb, 0x2, 0x2, 0x37, 0x38, 0x7, 0x23, 
       0x2, 0x2, 0x38, 0x39, 0x7, 0x3b, 0x2, 0x2, 0x39, 0x3c, 0x9, 0x2, 
       0x2, 0x2, 0x3a, 0x3b, 0x7, 0xe, 0x2, 0x2, 0x3b, 0x3d, 0x7, 0x24, 
       0x2, 0x2, 0x3c, 0x3a, 0x3, 0x2, 0x2, 0x2, 0x3c, 0x3d, 0x3, 0x2, 0x2, 
       0x2, 0x3d, 0x7, 0x3, 0x2, 0x2, 0x2, 0x3e, 0x3f, 0x7, 0x23, 0x2, 0x2, 
       0x3f, 0x45, 0x5, 0xa, 0x6, 0x2, 0x40, 0x41, 0x7, 0x3b, 0x2, 0x2, 
       0x41, 0x42, 0x7, 0x23, 0x2, 0x2, 0x42, 0x44, 0x5, 0xa, 0x6, 0x2, 
       0x43, 0x40, 0x3, 0x2, 0x2, 0x2, 0x44, 0x47, 0x3, 0x2, 0x2, 0x2, 0x45, 
       0x43, 0x3, 0x2, 0x2, 0x2, 0x45, 0x46, 0x3, 0x2, 0x2, 0x2, 0x46, 0x9, 
       0x3, 0x2, 0x2, 0x2, 0x47, 0x45, 0x3, 0x2, 0x2, 0x2, 0x48, 0x4e, 0x9, 
       0x3, 0x2, 0x2, 0x49, 0x4a, 0x9, 0x4, 0x2, 0x2, 0x4a, 0x4b, 0x7, 0x37, 
       0x2, 0x2, 0x4b, 0x4c, 0x7, 0x26, 0x2, 0x2, 0x4c, 0x4e, 0x7, 0x38, 
       0x2, 0x2, 0x4d, 0x48, 0x3, 0x2, 0x2, 0x2, 0x4d, 0x49, 0x3, 0x2, 0x2, 
       0x2, 0x4e, 0xb, 0x3, 0x2, 0x2, 0x2, 0x4f, 0x54, 0x5, 0xe, 0x8, 0x2, 
       0x50, 0x51, 0x7, 0x3b, 0x2, 0x2, 0x51, 0x53, 0x5, 0xe, 0x8, 0x2, 
       0x52, 0x50, 0x3, 0x2, 0x2, 0x2, 0x53, 0x56, 0x3, 0x2, 0x2, 0x2, 0x54, 
       0x52, 0x3, 0x2, 0x2, 0x2, 0x54, 0x55, 0x3, 0x2, 0x2, 0x2, 0x55, 0xd, 
       0x3, 0x2, 0x2, 0x2, 0x56, 0x54, 0x3, 0x2, 0x2, 0x2, 0x57, 0x5a, 0x5, 
       0x14, 0xb, 0x2, 0x58, 0x5a, 0x5, 0x16, 0xc, 0x2, 0x59, 0x57, 0x3, 
       0x2, 0x2, 0x2, 0x59, 0x58, 0x3, 0x2, 0x2, 0x2, 0x5a, 0xf, 0x3, 0x2, 
       0x2, 0x2, 0x5b, 0x68, 0x7, 0x23, 0x2, 0x2, 0x5c, 0x5d, 0x7, 0x23, 
       0x2, 0x2, 0x5d, 0x5e, 0x7, 0x37, 0x2, 0x2, 0x5e, 0x5f, 0x7, 0x2f, 
       0x2, 0x2, 0x5f, 0x68, 0x7, 0x38, 0x2, 0x2, 0x60, 0x61, 0x7, 0x23, 
       0x2, 0x2, 0x61, 0x62, 0x7, 0x2e, 0x2, 0x2, 0x62, 0x68, 0x7, 0x23, 
       0x2, 0x2, 0x63, 0x64, 0x7, 0x23, 0x2, 0x2, 0x64, 0x65, 0x7, 0x37, 
       0x2, 0x2, 0x65, 0x66, 0x7, 0x26, 0x2, 0x2, 0x66, 0x68, 0x7, 0x38, 
       0x2, 0x2, 0x67, 0x5b, 0x3, 0x2, 0x2, 0x2, 0x67, 0x5c, 0x3, 0x2, 0x2, 
       0x2, 0x67, 0x60, 0x3, 0x2, 0x2, 0x2, 0x67, 0x63, 0x3, 0x2, 0x2, 0x2, 
       0x68, 0x11, 0x3, 0x2, 0x2, 0x2, 0x69, 0x6a, 0x7, 0x43, 0x2, 0x2, 
       0x6a, 0x6e, 0x5, 0x16, 0xc, 0x2, 0x6b, 0x6c, 0x9, 0x5, 0x2, 0x2, 
       0x6c, 0x6e, 0x5, 0x16, 0xc, 0x2, 0x6d, 0x69, 0x3, 0x2, 0x2, 0x2, 
       0x6d, 0x6b, 0x3, 0x2, 0x2, 0x2, 0x6e, 0x13, 0x3, 0x2, 0x2, 0x2, 0x6f, 
       0x70, 0x7, 0x23, 0x2, 0x2, 0x70, 0x72, 0x7, 0x2e, 0x2, 0x2, 0x71, 
       0x6f, 0x3, 0x2, 0x2, 0x2, 0x71, 0x72, 0x3, 0x2, 0x2, 0x2, 0x72, 0x73, 
       0x3, 0x2, 0x2, 0x2, 0x73, 0x74, 0x7, 0x3f, 0x2, 0x2, 0x74, 0x15, 
       0x3, 0x2, 0x2, 0x2, 0x75, 0x7c, 0x5, 0x18, 0xd, 0x2, 0x76, 0x77, 
       0x7, 0x41, 0x2, 0x2, 0x77, 0x7b, 0x5, 0x18, 0xd, 0x2, 0x78, 0x79, 
       0x7, 0x42, 0x2, 0x2, 0x79, 0x7b, 0x5, 0x18, 0xd, 0x2, 0x7a, 0x76, 
       0x3, 0x2, 0x2, 0x2, 0x7a, 0x78, 0x3, 0x2, 0x2, 0x2, 0x7b, 0x7e, 0x3, 
       0x2, 0x2, 0x2, 0x7c, 0x7a, 0x3, 0x2, 0x2, 0x2, 0x7c, 0x7d, 0x3, 0x2, 
       0x2, 0x2, 0x7d, 0x17, 0x3, 0x2, 0x2, 0x2, 0x7e, 0x7c, 0x3, 0x2, 0x2, 
       0x2, 0x7f, 0x86, 0x5, 0x1a, 0xe, 0x2, 0x80, 0x81, 0x7, 0x3f, 0x2, 
       0x2, 0x81, 0x85, 0x5, 0x1a, 0xe, 0x2, 0x82, 0x83, 0x7, 0x40, 0x2, 
       0x2, 0x83, 0x85, 0x5, 0x1a, 0xe, 0x2, 0x84, 0x80, 0x3, 0x2, 0x2, 
       0x2, 0x84, 0x82, 0x3, 0x2, 0x2, 0x2, 0x85, 0x88, 0x3, 0x2, 0x2, 0x2, 
       0x86, 0x84, 0x3, 0x2, 0x2, 0x2, 0x86, 0x87, 0x3, 0x2, 0x2, 0x2, 0x87, 
       0x19, 0x3, 0x2, 0x2, 0x2, 0x88, 0x86, 0x3, 0x2, 0x2, 0x2, 0x89, 0x95, 
       0x7, 0x25, 0x2, 0x2, 0x8a, 0x95, 0x7, 0x28, 0x2, 0x2, 0x8b, 0x95, 
       0x7, 0x26, 0x2, 0x2, 0x8c, 0x95, 0x5, 0x12, 0xa, 0x2, 0x8d, 0x95, 
       0x5, 0x22, 0x12, 0x2, 0x8e, 0x95, 0x5, 0x10, 0x9, 0x2, 0x8f, 0x95, 
       0x5, 0x20, 0x11, 0x2, 0x90, 0x91, 0x7, 0x35, 0x2, 0x2, 0x91, 0x92, 
       0x5, 0x16, 0xc, 0x2, 0x92, 0x93, 0x7, 0x36, 0x2, 0x2, 0x93, 0x95, 
       0x3, 0x2, 0x2, 0x2, 0x94, 0x89, 0x3, 0x2, 0x2, 0x2, 0x94, 0x8a, 0x3, 
       0x2, 0x2, 0x2, 0x94, 0x8b, 0x3, 0x2, 0x2, 0x2, 0x94, 0x8c, 0x3, 0x2, 
       0x2, 0x2, 0x94, 0x8d, 0x3, 0x2, 0x2, 0x2, 0x94, 0x8e, 0x3, 0x2, 0x2, 
       0x2, 0x94, 0x8f, 0x3, 0x2, 0x2, 0x2, 0x94, 0x90, 0x3, 0x2, 0x2, 0x2, 
       0x95, 0x1b, 0x3, 0x2, 0x2, 0x2, 0x96, 0x9f, 0x5, 0x1e, 0x10, 0x2, 
       0x97, 0x98, 0x7, 0x2a, 0x2, 0x2, 0x98, 0x9e, 0x7, 0x26, 0x2, 0x2, 
       0x99, 0x9a, 0x7, 0x41, 0x2, 0x2, 0x9a, 0x9e, 0x5, 0x1e, 0x10, 0x2, 
       0x9b, 0x9c, 0x7, 0x42, 0x2, 0x2, 0x9c, 0x9e, 0x7, 0x27, 0x2, 0x2, 
       0x9d, 0x97, 0x3, 0x2, 0x2, 0x2, 0x9d, 0x99, 0x3, 0x2, 0x2, 0x2, 0x9d, 
       0x9b, 0x3, 0x2, 0x2, 0x2, 0x9e, 0xa1, 0x3, 0x2, 0x2, 0x2, 0x9f, 0x9d, 
       0x3, 0x2, 0x2, 0x2, 0x9f, 0xa0, 0x3, 0x2, 0x2, 0x2, 0xa0, 0x1d, 0x3, 
       0x2, 0x2, 0x2, 0xa1, 0x9f, 0x3, 0x2, 0x2, 0x2, 0xa2, 0xb6, 0x7, 0x23, 
       0x2, 0x2, 0xa3, 0xa4, 0x7, 0x31, 0x2, 0x2, 0xa4, 0xb5, 0x7, 0x23, 
       0x2, 0x2, 0xa5, 0xa6, 0x7, 0x32, 0x2, 0x2, 0xa6, 0xb5, 0x7, 0x27, 
       0x2, 0x2, 0xa7, 0xa8, 0x7, 0x33, 0x2, 0x2, 0xa8, 0xb5, 0x7, 0x27, 
       0x2, 0x2, 0xa9, 0xaa, 0x7, 0x30, 0x2, 0x2, 0xaa, 0xab, 0x7, 0x35, 
       0x2, 0x2, 0xab, 0xac, 0x7, 0x26, 0x2, 0x2, 0xac, 0xae, 0x7, 0x3b, 
       0x2, 0x2, 0xad, 0xaf, 0x9, 0x5, 0x2, 0x2, 0xae, 0xad, 0x3, 0x2, 0x2, 
       0x2, 0xae, 0xaf, 0x3, 0x2, 0x2, 0x2, 0xaf, 0xb0, 0x3, 0x2, 0x2, 0x2, 
       0xb0, 0xb1, 0x7, 0x26, 0x2, 0x2, 0xb1, 0xb5, 0x7, 0x36, 0x2, 0x2, 
       0xb2, 0xb3, 0x7, 0x2e, 0x2, 0x2, 0xb3, 0xb5, 0x5, 0x20, 0x11, 0x2, 
       0xb4, 0xa3, 0x3, 0x2, 0x2, 0x2, 0xb4, 0xa5, 0x3, 0x2, 0x2, 0x2, 0xb4, 
       0xa7, 0x3, 0x2, 0x2, 0x2, 0xb4, 0xa9, 0x3, 0x2, 0x2, 0x2, 0xb4, 0xb2, 
       0x3, 0x2, 0x2, 0x2, 0xb5, 0xb8, 0x3, 0x2, 0x2, 0x2, 0xb6, 0xb4, 0x3, 
       0x2, 0x2, 0x2, 0xb6, 0xb7, 0x3, 0x2, 0x2, 0x2, 0xb7, 0x1f, 0x3, 0x2, 
       0x2, 0x2, 0xb8, 0xb6, 0x3, 0x2, 0x2, 0x2, 0xb9, 0xba, 0x9, 0x6, 0x2, 
       0x2, 0xba, 0x21, 0x3, 0x2, 0x2, 0x2, 0xbb, 0xbc, 0x9, 0x7, 0x2, 0x2, 
       0xbc, 0xbd, 0x7, 0x35, 0x2, 0x2, 0xbd, 0xbe, 0x5, 0x16, 0xc, 0x2, 
       0xbe, 0xbf, 0x7, 0x36, 0x2, 0x2, 0xbf, 0x23, 0x3, 0x2, 0x2, 0x2, 
       0x16, 0x26, 0x28, 0x3c, 0x45, 0x4d, 0x54, 0x59, 0x67, 0x6d, 0x71, 
       0x7a, 0x7c, 0x84, 0x86, 0x94, 0x9d, 0x9f, 0xae, 0xb4, 0xb6, 
  };

  _serializedATN.insert(_serializedATN.end(), serializedATNSegment0,
    serializedATNSegment0 + sizeof(serializedATNSegment0) / sizeof(serializedATNSegment0[0]));


  atn::ATNDeserializer deserializer;
  _atn = deserializer.deserialize(_serializedATN);

  size_t count = _atn.getNumberOfDecisions();
  _decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    _decisionToDFA.emplace_back(_atn.getDecisionState(i), i);
  }
}

RQLParser::Initializer RQLParser::_init;
