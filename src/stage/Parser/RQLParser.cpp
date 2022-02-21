
// Generated from RQL.g4 by ANTLR 4.9.3


#include "RQLListener.h"

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

void RQLParser::ProgContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterProg(this);
}

void RQLParser::ProgContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitProg(this);
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
    setState(42); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(42);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::SELECT: {
          setState(40);
          select_statement();
          break;
        }

        case RQLParser::DECLARE: {
          setState(41);
          declare_statement();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(44); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == RQLParser::SELECT

    || _la == RQLParser::DECLARE);
    setState(46);
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


size_t RQLParser::Select_statementContext::getRuleIndex() const {
  return RQLParser::RuleSelect_statement;
}

void RQLParser::Select_statementContext::copyFrom(Select_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- SelectContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::SelectContext::SELECT() {
  return getToken(RQLParser::SELECT, 0);
}

RQLParser::Select_listContext* RQLParser::SelectContext::select_list() {
  return getRuleContext<RQLParser::Select_listContext>(0);
}

tree::TerminalNode* RQLParser::SelectContext::STREAM() {
  return getToken(RQLParser::STREAM, 0);
}

tree::TerminalNode* RQLParser::SelectContext::ID() {
  return getToken(RQLParser::ID, 0);
}

tree::TerminalNode* RQLParser::SelectContext::FROM() {
  return getToken(RQLParser::FROM, 0);
}

RQLParser::Stream_expressionContext* RQLParser::SelectContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

RQLParser::SelectContext::SelectContext(Select_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::SelectContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSelect(this);
}
void RQLParser::SelectContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSelect(this);
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
    _localctx = _tracker.createInstance<RQLParser::SelectContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(48);
    match(RQLParser::SELECT);
    setState(49);
    select_list();
    setState(50);
    match(RQLParser::STREAM);
    setState(51);
    match(RQLParser::ID);
    setState(52);
    match(RQLParser::FROM);
    setState(53);
    stream_expression();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- RationalContext ------------------------------------------------------------------

RQLParser::RationalContext::RationalContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::RationalContext::FLOAT() {
  return getToken(RQLParser::FLOAT, 0);
}

tree::TerminalNode* RQLParser::RationalContext::REAL() {
  return getToken(RQLParser::REAL, 0);
}

tree::TerminalNode* RQLParser::RationalContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

tree::TerminalNode* RQLParser::RationalContext::RATIONAL() {
  return getToken(RQLParser::RATIONAL, 0);
}


size_t RQLParser::RationalContext::getRuleIndex() const {
  return RQLParser::RuleRational;
}

void RQLParser::RationalContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRational(this);
}

void RQLParser::RationalContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRational(this);
}

RQLParser::RationalContext* RQLParser::rational() {
  RationalContext *_localctx = _tracker.createInstance<RationalContext>(_ctx, getState());
  enterRule(_localctx, 4, RQLParser::RuleRational);
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
    setState(55);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << RQLParser::FLOAT)
      | (1ULL << RQLParser::DECIMAL)
      | (1ULL << RQLParser::RATIONAL)
      | (1ULL << RQLParser::REAL))) != 0))) {
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

//----------------- Declare_statementContext ------------------------------------------------------------------

RQLParser::Declare_statementContext::Declare_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Declare_statementContext::getRuleIndex() const {
  return RQLParser::RuleDeclare_statement;
}

void RQLParser::Declare_statementContext::copyFrom(Declare_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- DeclareContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::DeclareContext::DECLARE() {
  return getToken(RQLParser::DECLARE, 0);
}

RQLParser::Declare_listContext* RQLParser::DeclareContext::declare_list() {
  return getRuleContext<RQLParser::Declare_listContext>(0);
}

tree::TerminalNode* RQLParser::DeclareContext::STREAM() {
  return getToken(RQLParser::STREAM, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::COMMA() {
  return getToken(RQLParser::COMMA, 0);
}

RQLParser::RationalContext* RQLParser::DeclareContext::rational() {
  return getRuleContext<RQLParser::RationalContext>(0);
}

tree::TerminalNode* RQLParser::DeclareContext::FILE() {
  return getToken(RQLParser::FILE, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::ID() {
  return getToken(RQLParser::ID, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::STRING() {
  return getToken(RQLParser::STRING, 0);
}

RQLParser::DeclareContext::DeclareContext(Declare_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::DeclareContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDeclare(this);
}
void RQLParser::DeclareContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDeclare(this);
}
RQLParser::Declare_statementContext* RQLParser::declare_statement() {
  Declare_statementContext *_localctx = _tracker.createInstance<Declare_statementContext>(_ctx, getState());
  enterRule(_localctx, 6, RQLParser::RuleDeclare_statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::DeclareContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(57);
    match(RQLParser::DECLARE);
    setState(58);
    declare_list();
    setState(59);
    match(RQLParser::STREAM);
    setState(60);
    antlrcpp::downCast<DeclareContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(61);
    match(RQLParser::COMMA);
    setState(62);
    rational();
    setState(63);
    match(RQLParser::FILE);
    setState(64);
    antlrcpp::downCast<DeclareContext *>(_localctx)->file_name = match(RQLParser::STRING);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Declare_listContext ------------------------------------------------------------------

RQLParser::Declare_listContext::Declare_listContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Declare_listContext::getRuleIndex() const {
  return RQLParser::RuleDeclare_list;
}

void RQLParser::Declare_listContext::copyFrom(Declare_listContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- DeclarationListContext ------------------------------------------------------------------

std::vector<RQLParser::Field_declarationContext *> RQLParser::DeclarationListContext::field_declaration() {
  return getRuleContexts<RQLParser::Field_declarationContext>();
}

RQLParser::Field_declarationContext* RQLParser::DeclarationListContext::field_declaration(size_t i) {
  return getRuleContext<RQLParser::Field_declarationContext>(i);
}

std::vector<tree::TerminalNode *> RQLParser::DeclarationListContext::COMMA() {
  return getTokens(RQLParser::COMMA);
}

tree::TerminalNode* RQLParser::DeclarationListContext::COMMA(size_t i) {
  return getToken(RQLParser::COMMA, i);
}

RQLParser::DeclarationListContext::DeclarationListContext(Declare_listContext *ctx) { copyFrom(ctx); }

void RQLParser::DeclarationListContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDeclarationList(this);
}
void RQLParser::DeclarationListContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDeclarationList(this);
}
RQLParser::Declare_listContext* RQLParser::declare_list() {
  Declare_listContext *_localctx = _tracker.createInstance<Declare_listContext>(_ctx, getState());
  enterRule(_localctx, 8, RQLParser::RuleDeclare_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::DeclarationListContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(66);
    field_declaration();
    setState(71);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(67);
      match(RQLParser::COMMA);
      setState(68);
      field_declaration();
      setState(73);
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

//----------------- Field_declarationContext ------------------------------------------------------------------

RQLParser::Field_declarationContext::Field_declarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Field_declarationContext::getRuleIndex() const {
  return RQLParser::RuleField_declaration;
}

void RQLParser::Field_declarationContext::copyFrom(Field_declarationContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- SingleDeclarationContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::SingleDeclarationContext::ID() {
  return getToken(RQLParser::ID, 0);
}

RQLParser::Field_typeContext* RQLParser::SingleDeclarationContext::field_type() {
  return getRuleContext<RQLParser::Field_typeContext>(0);
}

RQLParser::SingleDeclarationContext::SingleDeclarationContext(Field_declarationContext *ctx) { copyFrom(ctx); }

void RQLParser::SingleDeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSingleDeclaration(this);
}
void RQLParser::SingleDeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSingleDeclaration(this);
}
RQLParser::Field_declarationContext* RQLParser::field_declaration() {
  Field_declarationContext *_localctx = _tracker.createInstance<Field_declarationContext>(_ctx, getState());
  enterRule(_localctx, 10, RQLParser::RuleField_declaration);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::SingleDeclarationContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(74);
    match(RQLParser::ID);
    setState(75);
    field_type();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Field_typeContext ------------------------------------------------------------------

RQLParser::Field_typeContext::Field_typeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::Field_typeContext::FLOAT_T() {
  return getToken(RQLParser::FLOAT_T, 0);
}

tree::TerminalNode* RQLParser::Field_typeContext::BYTE_T() {
  return getToken(RQLParser::BYTE_T, 0);
}

tree::TerminalNode* RQLParser::Field_typeContext::INTEGER_T() {
  return getToken(RQLParser::INTEGER_T, 0);
}

tree::TerminalNode* RQLParser::Field_typeContext::UNSIGNED_T() {
  return getToken(RQLParser::UNSIGNED_T, 0);
}

tree::TerminalNode* RQLParser::Field_typeContext::STRING_T() {
  return getToken(RQLParser::STRING_T, 0);
}

tree::TerminalNode* RQLParser::Field_typeContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}


size_t RQLParser::Field_typeContext::getRuleIndex() const {
  return RQLParser::RuleField_type;
}

void RQLParser::Field_typeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterField_type(this);
}

void RQLParser::Field_typeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitField_type(this);
}

RQLParser::Field_typeContext* RQLParser::field_type() {
  Field_typeContext *_localctx = _tracker.createInstance<Field_typeContext>(_ctx, getState());
  enterRule(_localctx, 12, RQLParser::RuleField_type);
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
    setState(82);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 3, _ctx)) {
    case 1: {
      setState(77);
      _la = _input->LA(1);
      if (!((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & ((1ULL << RQLParser::STRING_T)
        | (1ULL << RQLParser::INTEGER_T)
        | (1ULL << RQLParser::BYTE_T))) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(78);
      match(RQLParser::T__0);
      setState(79);
      antlrcpp::downCast<Field_typeContext *>(_localctx)->type_size = match(RQLParser::DECIMAL);
      setState(80);
      match(RQLParser::T__1);
      break;
    }

    case 2: {
      setState(81);
      _la = _input->LA(1);
      if (!((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & ((1ULL << RQLParser::INTEGER_T)
        | (1ULL << RQLParser::BYTE_T)
        | (1ULL << RQLParser::UNSIGNED_T)
        | (1ULL << RQLParser::FLOAT_T))) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
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

//----------------- Select_listContext ------------------------------------------------------------------

RQLParser::Select_listContext::Select_listContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<RQLParser::Select_elemContext *> RQLParser::Select_listContext::select_elem() {
  return getRuleContexts<RQLParser::Select_elemContext>();
}

RQLParser::Select_elemContext* RQLParser::Select_listContext::select_elem(size_t i) {
  return getRuleContext<RQLParser::Select_elemContext>(i);
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

void RQLParser::Select_listContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSelect_list(this);
}

void RQLParser::Select_listContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSelect_list(this);
}

RQLParser::Select_listContext* RQLParser::select_list() {
  Select_listContext *_localctx = _tracker.createInstance<Select_listContext>(_ctx, getState());
  enterRule(_localctx, 14, RQLParser::RuleSelect_list);
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
    setState(84);
    select_elem();
    setState(89);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(85);
      match(RQLParser::COMMA);
      setState(86);
      select_elem();
      setState(91);
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

//----------------- Select_elemContext ------------------------------------------------------------------

RQLParser::Select_elemContext::Select_elemContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

RQLParser::AsteriskContext* RQLParser::Select_elemContext::asterisk() {
  return getRuleContext<RQLParser::AsteriskContext>(0);
}

RQLParser::ExpressionContext* RQLParser::Select_elemContext::expression() {
  return getRuleContext<RQLParser::ExpressionContext>(0);
}


size_t RQLParser::Select_elemContext::getRuleIndex() const {
  return RQLParser::RuleSelect_elem;
}

void RQLParser::Select_elemContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSelect_elem(this);
}

void RQLParser::Select_elemContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSelect_elem(this);
}

RQLParser::Select_elemContext* RQLParser::select_elem() {
  Select_elemContext *_localctx = _tracker.createInstance<Select_elemContext>(_ctx, getState());
  enterRule(_localctx, 16, RQLParser::RuleSelect_elem);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(94);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 5, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(92);
      asterisk();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(93);
      expression(0);
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


size_t RQLParser::Field_idContext::getRuleIndex() const {
  return RQLParser::RuleField_id;
}

void RQLParser::Field_idContext::copyFrom(Field_idContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- FieldIDUnderlineContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::FieldIDUnderlineContext::UNDERLINE() {
  return getToken(RQLParser::UNDERLINE, 0);
}

tree::TerminalNode* RQLParser::FieldIDUnderlineContext::ID() {
  return getToken(RQLParser::ID, 0);
}

RQLParser::FieldIDUnderlineContext::FieldIDUnderlineContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDUnderlineContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFieldIDUnderline(this);
}
void RQLParser::FieldIDUnderlineContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFieldIDUnderline(this);
}
//----------------- FieldIDTableContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::FieldIDTableContext::ID() {
  return getToken(RQLParser::ID, 0);
}

tree::TerminalNode* RQLParser::FieldIDTableContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

RQLParser::FieldIDTableContext::FieldIDTableContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDTableContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFieldIDTable(this);
}
void RQLParser::FieldIDTableContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFieldIDTable(this);
}
//----------------- FieldIDContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::FieldIDContext::ID() {
  return getToken(RQLParser::ID, 0);
}

RQLParser::FieldIDContext::FieldIDContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFieldID(this);
}
void RQLParser::FieldIDContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFieldID(this);
}
//----------------- FieldIDColumnnameContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::FieldIDColumnnameContext::DOT() {
  return getToken(RQLParser::DOT, 0);
}

std::vector<tree::TerminalNode *> RQLParser::FieldIDColumnnameContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::FieldIDColumnnameContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

RQLParser::FieldIDColumnnameContext::FieldIDColumnnameContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDColumnnameContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFieldIDColumnname(this);
}
void RQLParser::FieldIDColumnnameContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFieldIDColumnname(this);
}
RQLParser::Field_idContext* RQLParser::field_id() {
  Field_idContext *_localctx = _tracker.createInstance<Field_idContext>(_ctx, getState());
  enterRule(_localctx, 18, RQLParser::RuleField_id);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(108);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 6, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(96);
      antlrcpp::downCast<FieldIDContext *>(_localctx)->column_name = match(RQLParser::ID);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDUnderlineContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(97);
      antlrcpp::downCast<FieldIDUnderlineContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(98);
      match(RQLParser::T__0);
      setState(99);
      match(RQLParser::UNDERLINE);
      setState(100);
      match(RQLParser::T__1);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDColumnnameContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(101);
      antlrcpp::downCast<FieldIDColumnnameContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(102);
      match(RQLParser::DOT);
      setState(103);
      antlrcpp::downCast<FieldIDColumnnameContext *>(_localctx)->column_name = match(RQLParser::ID);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDTableContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(104);
      antlrcpp::downCast<FieldIDTableContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(105);
      match(RQLParser::T__0);
      setState(106);
      antlrcpp::downCast<FieldIDTableContext *>(_localctx)->column_index = match(RQLParser::DECIMAL);
      setState(107);
      match(RQLParser::T__1);
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

void RQLParser::Unary_op_expressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnary_op_expression(this);
}

void RQLParser::Unary_op_expressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnary_op_expression(this);
}

RQLParser::Unary_op_expressionContext* RQLParser::unary_op_expression() {
  Unary_op_expressionContext *_localctx = _tracker.createInstance<Unary_op_expressionContext>(_ctx, getState());
  enterRule(_localctx, 20, RQLParser::RuleUnary_op_expression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(114);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::BIT_NOT: {
        enterOuterAlt(_localctx, 1);
        setState(110);
        match(RQLParser::BIT_NOT);
        setState(111);
        expression(0);
        break;
      }

      case RQLParser::PLUS:
      case RQLParser::MINUS: {
        enterOuterAlt(_localctx, 2);
        setState(112);
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
        setState(113);
        expression(0);
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

void RQLParser::AsteriskContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAsterisk(this);
}

void RQLParser::AsteriskContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAsterisk(this);
}

RQLParser::AsteriskContext* RQLParser::asterisk() {
  AsteriskContext *_localctx = _tracker.createInstance<AsteriskContext>(_ctx, getState());
  enterRule(_localctx, 22, RQLParser::RuleAsterisk);
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
    setState(118);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::ID) {
      setState(116);
      match(RQLParser::ID);
      setState(117);
      match(RQLParser::DOT);
    }
    setState(120);
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

RQLParser::TermContext* RQLParser::ExpressionContext::term() {
  return getRuleContext<RQLParser::TermContext>(0);
}

std::vector<RQLParser::ExpressionContext *> RQLParser::ExpressionContext::expression() {
  return getRuleContexts<RQLParser::ExpressionContext>();
}

RQLParser::ExpressionContext* RQLParser::ExpressionContext::expression(size_t i) {
  return getRuleContext<RQLParser::ExpressionContext>(i);
}

tree::TerminalNode* RQLParser::ExpressionContext::PLUS() {
  return getToken(RQLParser::PLUS, 0);
}

tree::TerminalNode* RQLParser::ExpressionContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}


size_t RQLParser::ExpressionContext::getRuleIndex() const {
  return RQLParser::RuleExpression;
}

void RQLParser::ExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpression(this);
}

void RQLParser::ExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpression(this);
}


RQLParser::ExpressionContext* RQLParser::expression() {
   return expression(0);
}

RQLParser::ExpressionContext* RQLParser::expression(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  RQLParser::ExpressionContext *_localctx = _tracker.createInstance<ExpressionContext>(_ctx, parentState);
  RQLParser::ExpressionContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 24;
  enterRecursionRule(_localctx, 24, RQLParser::RuleExpression, precedence);

    

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(123);
    term(0);
    _ctx->stop = _input->LT(-1);
    setState(133);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 10, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(131);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx)) {
        case 1: {
          _localctx = _tracker.createInstance<ExpressionContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleExpression);
          setState(125);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(126);
          match(RQLParser::PLUS);
          setState(127);
          expression(4);
          break;
        }

        case 2: {
          _localctx = _tracker.createInstance<ExpressionContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleExpression);
          setState(128);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(129);
          match(RQLParser::MINUS);
          setState(130);
          expression(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(135);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 10, _ctx);
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

RQLParser::ExpressionContext* RQLParser::TermContext::expression() {
  return getRuleContext<RQLParser::ExpressionContext>(0);
}

RQLParser::FactorContext* RQLParser::TermContext::factor() {
  return getRuleContext<RQLParser::FactorContext>(0);
}

std::vector<RQLParser::TermContext *> RQLParser::TermContext::term() {
  return getRuleContexts<RQLParser::TermContext>();
}

RQLParser::TermContext* RQLParser::TermContext::term(size_t i) {
  return getRuleContext<RQLParser::TermContext>(i);
}

tree::TerminalNode* RQLParser::TermContext::STAR() {
  return getToken(RQLParser::STAR, 0);
}

tree::TerminalNode* RQLParser::TermContext::DIVIDE() {
  return getToken(RQLParser::DIVIDE, 0);
}


size_t RQLParser::TermContext::getRuleIndex() const {
  return RQLParser::RuleTerm;
}

void RQLParser::TermContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTerm(this);
}

void RQLParser::TermContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTerm(this);
}


RQLParser::TermContext* RQLParser::term() {
   return term(0);
}

RQLParser::TermContext* RQLParser::term(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  RQLParser::TermContext *_localctx = _tracker.createInstance<TermContext>(_ctx, parentState);
  RQLParser::TermContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 26;
  enterRecursionRule(_localctx, 26, RQLParser::RuleTerm, precedence);

    

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(142);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::T__2: {
        setState(137);
        match(RQLParser::T__2);
        setState(138);
        expression(0);
        setState(139);
        match(RQLParser::T__3);
        break;
      }

      case RQLParser::MIN:
      case RQLParser::MAX:
      case RQLParser::AVG:
      case RQLParser::SUMC:
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
      case RQLParser::ISNONZERO:
      case RQLParser::ID:
      case RQLParser::FLOAT:
      case RQLParser::DECIMAL:
      case RQLParser::REAL:
      case RQLParser::PLUS:
      case RQLParser::MINUS:
      case RQLParser::BIT_NOT: {
        setState(141);
        antlrcpp::downCast<TermContext *>(_localctx)->t = factor();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    _ctx->stop = _input->LT(-1);
    setState(152);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 13, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(150);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 12, _ctx)) {
        case 1: {
          _localctx = _tracker.createInstance<TermContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleTerm);
          setState(144);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(145);
          match(RQLParser::STAR);
          setState(146);
          term(5);
          break;
        }

        case 2: {
          _localctx = _tracker.createInstance<TermContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleTerm);
          setState(147);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(148);
          match(RQLParser::DIVIDE);
          setState(149);
          term(4);
          break;
        }

        default:
          break;
        } 
      }
      setState(154);
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


size_t RQLParser::FactorContext::getRuleIndex() const {
  return RQLParser::RuleFactor;
}

void RQLParser::FactorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFactor(this);
}

void RQLParser::FactorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFactor(this);
}

RQLParser::FactorContext* RQLParser::factor() {
  FactorContext *_localctx = _tracker.createInstance<FactorContext>(_ctx, getState());
  enterRule(_localctx, 28, RQLParser::RuleFactor);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(162);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::FLOAT: {
        enterOuterAlt(_localctx, 1);
        setState(155);
        antlrcpp::downCast<FactorContext *>(_localctx)->w = match(RQLParser::FLOAT);
        break;
      }

      case RQLParser::REAL: {
        enterOuterAlt(_localctx, 2);
        setState(156);
        antlrcpp::downCast<FactorContext *>(_localctx)->w = match(RQLParser::REAL);
        break;
      }

      case RQLParser::DECIMAL: {
        enterOuterAlt(_localctx, 3);
        setState(157);
        antlrcpp::downCast<FactorContext *>(_localctx)->w = match(RQLParser::DECIMAL);
        break;
      }

      case RQLParser::PLUS:
      case RQLParser::MINUS:
      case RQLParser::BIT_NOT: {
        enterOuterAlt(_localctx, 4);
        setState(158);
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
        setState(159);
        function_call();
        break;
      }

      case RQLParser::ID: {
        enterOuterAlt(_localctx, 6);
        setState(160);
        field_id();
        break;
      }

      case RQLParser::MIN:
      case RQLParser::MAX:
      case RQLParser::AVG:
      case RQLParser::SUMC: {
        enterOuterAlt(_localctx, 7);
        setState(161);
        agregator();
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

std::vector<tree::TerminalNode *> RQLParser::Stream_expressionContext::DECIMAL() {
  return getTokens(RQLParser::DECIMAL);
}

tree::TerminalNode* RQLParser::Stream_expressionContext::DECIMAL(size_t i) {
  return getToken(RQLParser::DECIMAL, i);
}

std::vector<RQLParser::RationalContext *> RQLParser::Stream_expressionContext::rational() {
  return getRuleContexts<RQLParser::RationalContext>();
}

RQLParser::RationalContext* RQLParser::Stream_expressionContext::rational(size_t i) {
  return getRuleContext<RQLParser::RationalContext>(i);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_expressionContext::GREATER() {
  return getTokens(RQLParser::GREATER);
}

tree::TerminalNode* RQLParser::Stream_expressionContext::GREATER(size_t i) {
  return getToken(RQLParser::GREATER, i);
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


size_t RQLParser::Stream_expressionContext::getRuleIndex() const {
  return RQLParser::RuleStream_expression;
}

void RQLParser::Stream_expressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStream_expression(this);
}

void RQLParser::Stream_expressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStream_expression(this);
}

RQLParser::Stream_expressionContext* RQLParser::stream_expression() {
  Stream_expressionContext *_localctx = _tracker.createInstance<Stream_expressionContext>(_ctx, getState());
  enterRule(_localctx, 30, RQLParser::RuleStream_expression);
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
    setState(164);
    stream_term();
    setState(173);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << RQLParser::GREATER)
      | (1ULL << RQLParser::PLUS)
      | (1ULL << RQLParser::MINUS))) != 0)) {
      setState(171);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::GREATER: {
          setState(165);
          antlrcpp::downCast<Stream_expressionContext *>(_localctx)->timemove = match(RQLParser::GREATER);
          setState(166);
          match(RQLParser::DECIMAL);
          break;
        }

        case RQLParser::PLUS: {
          setState(167);
          antlrcpp::downCast<Stream_expressionContext *>(_localctx)->stream_add = match(RQLParser::PLUS);
          setState(168);
          stream_term();
          break;
        }

        case RQLParser::MINUS: {
          setState(169);
          antlrcpp::downCast<Stream_expressionContext *>(_localctx)->stream_sub = match(RQLParser::MINUS);
          setState(170);
          rational();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(175);
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

RQLParser::Stream_factorContext* RQLParser::Stream_termContext::stream_factor() {
  return getRuleContext<RQLParser::Stream_factorContext>(0);
}

std::vector<tree::TerminalNode *> RQLParser::Stream_termContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::Stream_termContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

std::vector<RQLParser::RationalContext *> RQLParser::Stream_termContext::rational() {
  return getRuleContexts<RQLParser::RationalContext>();
}

RQLParser::RationalContext* RQLParser::Stream_termContext::rational(size_t i) {
  return getRuleContext<RQLParser::RationalContext>(i);
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

void RQLParser::Stream_termContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStream_term(this);
}

void RQLParser::Stream_termContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStream_term(this);
}

RQLParser::Stream_termContext* RQLParser::stream_term() {
  Stream_termContext *_localctx = _tracker.createInstance<Stream_termContext>(_ctx, getState());
  enterRule(_localctx, 32, RQLParser::RuleStream_term);
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
    setState(176);
    stream_factor();
    setState(196);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << RQLParser::DOT)
      | (1ULL << RQLParser::AT)
      | (1ULL << RQLParser::SHARP)
      | (1ULL << RQLParser::AND)
      | (1ULL << RQLParser::MOD))) != 0)) {
      setState(194);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::SHARP: {
          setState(177);
          antlrcpp::downCast<Stream_termContext *>(_localctx)->hash = match(RQLParser::SHARP);
          setState(178);
          match(RQLParser::ID);
          break;
        }

        case RQLParser::AND: {
          setState(179);
          antlrcpp::downCast<Stream_termContext *>(_localctx)->dehash_div = match(RQLParser::AND);
          setState(180);
          rational();
          break;
        }

        case RQLParser::MOD: {
          setState(181);
          antlrcpp::downCast<Stream_termContext *>(_localctx)->dehash_mod = match(RQLParser::MOD);
          setState(182);
          rational();
          break;
        }

        case RQLParser::AT: {
          setState(183);
          antlrcpp::downCast<Stream_termContext *>(_localctx)->agse = match(RQLParser::AT);
          setState(184);
          match(RQLParser::T__2);
          setState(185);
          match(RQLParser::DECIMAL);
          setState(186);
          match(RQLParser::COMMA);
          setState(188);
          _errHandler->sync(this);

          _la = _input->LA(1);
          if (_la == RQLParser::PLUS

          || _la == RQLParser::MINUS) {
            setState(187);
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
          setState(190);
          match(RQLParser::DECIMAL);
          setState(191);
          match(RQLParser::T__3);
          break;
        }

        case RQLParser::DOT: {
          setState(192);
          match(RQLParser::DOT);
          setState(193);
          agregator();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(198);
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

//----------------- Stream_factorContext ------------------------------------------------------------------

RQLParser::Stream_factorContext::Stream_factorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::Stream_factorContext::ID() {
  return getToken(RQLParser::ID, 0);
}

RQLParser::Stream_expressionContext* RQLParser::Stream_factorContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}


size_t RQLParser::Stream_factorContext::getRuleIndex() const {
  return RQLParser::RuleStream_factor;
}

void RQLParser::Stream_factorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStream_factor(this);
}

void RQLParser::Stream_factorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStream_factor(this);
}

RQLParser::Stream_factorContext* RQLParser::stream_factor() {
  Stream_factorContext *_localctx = _tracker.createInstance<Stream_factorContext>(_ctx, getState());
  enterRule(_localctx, 34, RQLParser::RuleStream_factor);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(204);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::ID: {
        enterOuterAlt(_localctx, 1);
        setState(199);
        match(RQLParser::ID);
        break;
      }

      case RQLParser::T__2: {
        enterOuterAlt(_localctx, 2);
        setState(200);
        match(RQLParser::T__2);
        setState(201);
        stream_expression();
        setState(202);
        match(RQLParser::T__3);
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

void RQLParser::AgregatorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAgregator(this);
}

void RQLParser::AgregatorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAgregator(this);
}

RQLParser::AgregatorContext* RQLParser::agregator() {
  AgregatorContext *_localctx = _tracker.createInstance<AgregatorContext>(_ctx, getState());
  enterRule(_localctx, 36, RQLParser::RuleAgregator);
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
    setState(206);
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

RQLParser::ExpressionContext* RQLParser::Function_callContext::expression() {
  return getRuleContext<RQLParser::ExpressionContext>(0);
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

void RQLParser::Function_callContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFunction_call(this);
}

void RQLParser::Function_callContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFunction_call(this);
}

RQLParser::Function_callContext* RQLParser::function_call() {
  Function_callContext *_localctx = _tracker.createInstance<Function_callContext>(_ctx, getState());
  enterRule(_localctx, 38, RQLParser::RuleFunction_call);
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
    setState(208);
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
    setState(209);
    match(RQLParser::T__2);
    setState(210);
    expression(0);
    setState(211);
    match(RQLParser::T__3);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

bool RQLParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 12: return expressionSempred(antlrcpp::downCast<ExpressionContext *>(context), predicateIndex);
    case 13: return termSempred(antlrcpp::downCast<TermContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool RQLParser::expressionSempred(ExpressionContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 3);
    case 1: return precpred(_ctx, 2);

  default:
    break;
  }
  return true;
}

bool RQLParser::termSempred(TermContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 2: return precpred(_ctx, 4);
    case 3: return precpred(_ctx, 3);

  default:
    break;
  }
  return true;
}

// Static vars and initialization.
std::vector<dfa::DFA> RQLParser::_decisionToDFA;
atn::PredictionContextCache RQLParser::_sharedContextCache;

// We own the ATN which in turn owns the ATN states.
atn::ATN RQLParser::_atn;
std::vector<uint16_t> RQLParser::_serializedATN;

std::vector<std::string> RQLParser::_ruleNames = {
  "prog", "select_statement", "rational", "declare_statement", "declare_list", 
  "field_declaration", "field_type", "select_list", "select_elem", "field_id", 
  "unary_op_expression", "asterisk", "expression", "term", "factor", "stream_expression", 
  "stream_term", "stream_factor", "agregator", "function_call"
};

std::vector<std::string> RQLParser::_literalNames = {
  "", "'['", "']'", "'('", "')'", "'STRING'", "'INT'", "'BYTE'", "'UNSIGNED'", 
  "'FLOAT'", "'DOUBLE'", "'SELECT'", "'STREAM'", "'FROM'", "'DECLARE'", 
  "'FILE'", "'MIN'", "'MAX'", "'AVG'", "'SUMC'", "'Sqrt'", "'Ceil'", "'Abs'", 
  "'Floor'", "'Sign'", "'Chr'", "'Length'", "'ToNumber'", "'ToTimeStamp'", 
  "'FloatCast'", "'InstCast'", "'Count'", "'Crc'", "'Sum'", "'IsZero'", 
  "'IsNonZero'", "", "", "", "", "", "", "'='", "'>'", "'<'", "'!'", "'||'", 
  "'.'", "'_'", "'@'", "'#'", "'&'", "'%'", "'$'", "','", "';'", "':'", 
  "'::'", "'*'", "'/'", "'+'", "'-'", "'~'", "'|'", "'^'"
};

std::vector<std::string> RQLParser::_symbolicNames = {
  "", "", "", "", "", "STRING_T", "INTEGER_T", "BYTE_T", "UNSIGNED_T", "FLOAT_T", 
  "DOUBLE_T", "SELECT", "STREAM", "FROM", "DECLARE", "FILE", "MIN", "MAX", 
  "AVG", "SUMC", "SQRT", "CEIL", "ABS", "FLOOR", "SIGN", "CHR", "LENGTH", 
  "TO_NUMBER", "TO_TIMESTAMP", "FLOAT_CAST", "INT_CAST", "COUNT", "CRC", 
  "SUM", "ISZERO", "ISNONZERO", "ID", "STRING", "FLOAT", "DECIMAL", "RATIONAL", 
  "REAL", "EQUAL", "GREATER", "LESS", "EXCLAMATION", "DOUBLE_BAR", "DOT", 
  "UNDERLINE", "AT", "SHARP", "AND", "MOD", "DOLLAR", "COMMA", "SEMI", "COLON", 
  "DOUBLE_COLON", "STAR", "DIVIDE", "PLUS", "MINUS", "BIT_NOT", "BIT_OR", 
  "BIT_XOR", "SPACE", "COMMENT", "LINE_COMMENT"
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
       0x3, 0x45, 0xd8, 0x4, 0x2, 0x9, 0x2, 0x4, 0x3, 0x9, 0x3, 0x4, 0x4, 
       0x9, 0x4, 0x4, 0x5, 0x9, 0x5, 0x4, 0x6, 0x9, 0x6, 0x4, 0x7, 0x9, 
       0x7, 0x4, 0x8, 0x9, 0x8, 0x4, 0x9, 0x9, 0x9, 0x4, 0xa, 0x9, 0xa, 
       0x4, 0xb, 0x9, 0xb, 0x4, 0xc, 0x9, 0xc, 0x4, 0xd, 0x9, 0xd, 0x4, 
       0xe, 0x9, 0xe, 0x4, 0xf, 0x9, 0xf, 0x4, 0x10, 0x9, 0x10, 0x4, 0x11, 
       0x9, 0x11, 0x4, 0x12, 0x9, 0x12, 0x4, 0x13, 0x9, 0x13, 0x4, 0x14, 
       0x9, 0x14, 0x4, 0x15, 0x9, 0x15, 0x3, 0x2, 0x3, 0x2, 0x6, 0x2, 0x2d, 
       0xa, 0x2, 0xd, 0x2, 0xe, 0x2, 0x2e, 0x3, 0x2, 0x3, 0x2, 0x3, 0x3, 
       0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 
       0x4, 0x3, 0x4, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 
       0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x3, 0x6, 0x3, 0x6, 0x3, 
       0x6, 0x7, 0x6, 0x48, 0xa, 0x6, 0xc, 0x6, 0xe, 0x6, 0x4b, 0xb, 0x6, 
       0x3, 0x7, 0x3, 0x7, 0x3, 0x7, 0x3, 0x8, 0x3, 0x8, 0x3, 0x8, 0x3, 
       0x8, 0x3, 0x8, 0x5, 0x8, 0x55, 0xa, 0x8, 0x3, 0x9, 0x3, 0x9, 0x3, 
       0x9, 0x7, 0x9, 0x5a, 0xa, 0x9, 0xc, 0x9, 0xe, 0x9, 0x5d, 0xb, 0x9, 
       0x3, 0xa, 0x3, 0xa, 0x5, 0xa, 0x61, 0xa, 0xa, 0x3, 0xb, 0x3, 0xb, 
       0x3, 0xb, 0x3, 0xb, 0x3, 0xb, 0x3, 0xb, 0x3, 0xb, 0x3, 0xb, 0x3, 
       0xb, 0x3, 0xb, 0x3, 0xb, 0x3, 0xb, 0x5, 0xb, 0x6f, 0xa, 0xb, 0x3, 
       0xc, 0x3, 0xc, 0x3, 0xc, 0x3, 0xc, 0x5, 0xc, 0x75, 0xa, 0xc, 0x3, 
       0xd, 0x3, 0xd, 0x5, 0xd, 0x79, 0xa, 0xd, 0x3, 0xd, 0x3, 0xd, 0x3, 
       0xe, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 0x3, 0xe, 
       0x3, 0xe, 0x3, 0xe, 0x7, 0xe, 0x86, 0xa, 0xe, 0xc, 0xe, 0xe, 0xe, 
       0x89, 0xb, 0xe, 0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 
       0x3, 0xf, 0x5, 0xf, 0x91, 0xa, 0xf, 0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 
       0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 0x7, 0xf, 0x99, 0xa, 0xf, 0xc, 0xf, 
       0xe, 0xf, 0x9c, 0xb, 0xf, 0x3, 0x10, 0x3, 0x10, 0x3, 0x10, 0x3, 0x10, 
       0x3, 0x10, 0x3, 0x10, 0x3, 0x10, 0x5, 0x10, 0xa5, 0xa, 0x10, 0x3, 
       0x11, 0x3, 0x11, 0x3, 0x11, 0x3, 0x11, 0x3, 0x11, 0x3, 0x11, 0x3, 
       0x11, 0x7, 0x11, 0xae, 0xa, 0x11, 0xc, 0x11, 0xe, 0x11, 0xb1, 0xb, 
       0x11, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 
       0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 
       0x12, 0x5, 0x12, 0xbf, 0xa, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 
       0x3, 0x12, 0x7, 0x12, 0xc5, 0xa, 0x12, 0xc, 0x12, 0xe, 0x12, 0xc8, 
       0xb, 0x12, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 
       0x5, 0x13, 0xcf, 0xa, 0x13, 0x3, 0x14, 0x3, 0x14, 0x3, 0x15, 0x3, 
       0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x3, 0x15, 0x2, 0x4, 0x1a, 
       0x1c, 0x16, 0x2, 0x4, 0x6, 0x8, 0xa, 0xc, 0xe, 0x10, 0x12, 0x14, 
       0x16, 0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2, 
       0x8, 0x3, 0x2, 0x28, 0x2b, 0x3, 0x2, 0x7, 0x9, 0x3, 0x2, 0x8, 0xb, 
       0x3, 0x2, 0x3e, 0x3f, 0x3, 0x2, 0x12, 0x15, 0x3, 0x2, 0x16, 0x25, 
       0x2, 0xe3, 0x2, 0x2c, 0x3, 0x2, 0x2, 0x2, 0x4, 0x32, 0x3, 0x2, 0x2, 
       0x2, 0x6, 0x39, 0x3, 0x2, 0x2, 0x2, 0x8, 0x3b, 0x3, 0x2, 0x2, 0x2, 
       0xa, 0x44, 0x3, 0x2, 0x2, 0x2, 0xc, 0x4c, 0x3, 0x2, 0x2, 0x2, 0xe, 
       0x54, 0x3, 0x2, 0x2, 0x2, 0x10, 0x56, 0x3, 0x2, 0x2, 0x2, 0x12, 0x60, 
       0x3, 0x2, 0x2, 0x2, 0x14, 0x6e, 0x3, 0x2, 0x2, 0x2, 0x16, 0x74, 0x3, 
       0x2, 0x2, 0x2, 0x18, 0x78, 0x3, 0x2, 0x2, 0x2, 0x1a, 0x7c, 0x3, 0x2, 
       0x2, 0x2, 0x1c, 0x90, 0x3, 0x2, 0x2, 0x2, 0x1e, 0xa4, 0x3, 0x2, 0x2, 
       0x2, 0x20, 0xa6, 0x3, 0x2, 0x2, 0x2, 0x22, 0xb2, 0x3, 0x2, 0x2, 0x2, 
       0x24, 0xce, 0x3, 0x2, 0x2, 0x2, 0x26, 0xd0, 0x3, 0x2, 0x2, 0x2, 0x28, 
       0xd2, 0x3, 0x2, 0x2, 0x2, 0x2a, 0x2d, 0x5, 0x4, 0x3, 0x2, 0x2b, 0x2d, 
       0x5, 0x8, 0x5, 0x2, 0x2c, 0x2a, 0x3, 0x2, 0x2, 0x2, 0x2c, 0x2b, 0x3, 
       0x2, 0x2, 0x2, 0x2d, 0x2e, 0x3, 0x2, 0x2, 0x2, 0x2e, 0x2c, 0x3, 0x2, 
       0x2, 0x2, 0x2e, 0x2f, 0x3, 0x2, 0x2, 0x2, 0x2f, 0x30, 0x3, 0x2, 0x2, 
       0x2, 0x30, 0x31, 0x7, 0x2, 0x2, 0x3, 0x31, 0x3, 0x3, 0x2, 0x2, 0x2, 
       0x32, 0x33, 0x7, 0xd, 0x2, 0x2, 0x33, 0x34, 0x5, 0x10, 0x9, 0x2, 
       0x34, 0x35, 0x7, 0xe, 0x2, 0x2, 0x35, 0x36, 0x7, 0x26, 0x2, 0x2, 
       0x36, 0x37, 0x7, 0xf, 0x2, 0x2, 0x37, 0x38, 0x5, 0x20, 0x11, 0x2, 
       0x38, 0x5, 0x3, 0x2, 0x2, 0x2, 0x39, 0x3a, 0x9, 0x2, 0x2, 0x2, 0x3a, 
       0x7, 0x3, 0x2, 0x2, 0x2, 0x3b, 0x3c, 0x7, 0x10, 0x2, 0x2, 0x3c, 0x3d, 
       0x5, 0xa, 0x6, 0x2, 0x3d, 0x3e, 0x7, 0xe, 0x2, 0x2, 0x3e, 0x3f, 0x7, 
       0x26, 0x2, 0x2, 0x3f, 0x40, 0x7, 0x38, 0x2, 0x2, 0x40, 0x41, 0x5, 
       0x6, 0x4, 0x2, 0x41, 0x42, 0x7, 0x11, 0x2, 0x2, 0x42, 0x43, 0x7, 
       0x27, 0x2, 0x2, 0x43, 0x9, 0x3, 0x2, 0x2, 0x2, 0x44, 0x49, 0x5, 0xc, 
       0x7, 0x2, 0x45, 0x46, 0x7, 0x38, 0x2, 0x2, 0x46, 0x48, 0x5, 0xc, 
       0x7, 0x2, 0x47, 0x45, 0x3, 0x2, 0x2, 0x2, 0x48, 0x4b, 0x3, 0x2, 0x2, 
       0x2, 0x49, 0x47, 0x3, 0x2, 0x2, 0x2, 0x49, 0x4a, 0x3, 0x2, 0x2, 0x2, 
       0x4a, 0xb, 0x3, 0x2, 0x2, 0x2, 0x4b, 0x49, 0x3, 0x2, 0x2, 0x2, 0x4c, 
       0x4d, 0x7, 0x26, 0x2, 0x2, 0x4d, 0x4e, 0x5, 0xe, 0x8, 0x2, 0x4e, 
       0xd, 0x3, 0x2, 0x2, 0x2, 0x4f, 0x50, 0x9, 0x3, 0x2, 0x2, 0x50, 0x51, 
       0x7, 0x3, 0x2, 0x2, 0x51, 0x52, 0x7, 0x29, 0x2, 0x2, 0x52, 0x55, 
       0x7, 0x4, 0x2, 0x2, 0x53, 0x55, 0x9, 0x4, 0x2, 0x2, 0x54, 0x4f, 0x3, 
       0x2, 0x2, 0x2, 0x54, 0x53, 0x3, 0x2, 0x2, 0x2, 0x55, 0xf, 0x3, 0x2, 
       0x2, 0x2, 0x56, 0x5b, 0x5, 0x12, 0xa, 0x2, 0x57, 0x58, 0x7, 0x38, 
       0x2, 0x2, 0x58, 0x5a, 0x5, 0x12, 0xa, 0x2, 0x59, 0x57, 0x3, 0x2, 
       0x2, 0x2, 0x5a, 0x5d, 0x3, 0x2, 0x2, 0x2, 0x5b, 0x59, 0x3, 0x2, 0x2, 
       0x2, 0x5b, 0x5c, 0x3, 0x2, 0x2, 0x2, 0x5c, 0x11, 0x3, 0x2, 0x2, 0x2, 
       0x5d, 0x5b, 0x3, 0x2, 0x2, 0x2, 0x5e, 0x61, 0x5, 0x18, 0xd, 0x2, 
       0x5f, 0x61, 0x5, 0x1a, 0xe, 0x2, 0x60, 0x5e, 0x3, 0x2, 0x2, 0x2, 
       0x60, 0x5f, 0x3, 0x2, 0x2, 0x2, 0x61, 0x13, 0x3, 0x2, 0x2, 0x2, 0x62, 
       0x6f, 0x7, 0x26, 0x2, 0x2, 0x63, 0x64, 0x7, 0x26, 0x2, 0x2, 0x64, 
       0x65, 0x7, 0x3, 0x2, 0x2, 0x65, 0x66, 0x7, 0x32, 0x2, 0x2, 0x66, 
       0x6f, 0x7, 0x4, 0x2, 0x2, 0x67, 0x68, 0x7, 0x26, 0x2, 0x2, 0x68, 
       0x69, 0x7, 0x31, 0x2, 0x2, 0x69, 0x6f, 0x7, 0x26, 0x2, 0x2, 0x6a, 
       0x6b, 0x7, 0x26, 0x2, 0x2, 0x6b, 0x6c, 0x7, 0x3, 0x2, 0x2, 0x6c, 
       0x6d, 0x7, 0x29, 0x2, 0x2, 0x6d, 0x6f, 0x7, 0x4, 0x2, 0x2, 0x6e, 
       0x62, 0x3, 0x2, 0x2, 0x2, 0x6e, 0x63, 0x3, 0x2, 0x2, 0x2, 0x6e, 0x67, 
       0x3, 0x2, 0x2, 0x2, 0x6e, 0x6a, 0x3, 0x2, 0x2, 0x2, 0x6f, 0x15, 0x3, 
       0x2, 0x2, 0x2, 0x70, 0x71, 0x7, 0x40, 0x2, 0x2, 0x71, 0x75, 0x5, 
       0x1a, 0xe, 0x2, 0x72, 0x73, 0x9, 0x5, 0x2, 0x2, 0x73, 0x75, 0x5, 
       0x1a, 0xe, 0x2, 0x74, 0x70, 0x3, 0x2, 0x2, 0x2, 0x74, 0x72, 0x3, 
       0x2, 0x2, 0x2, 0x75, 0x17, 0x3, 0x2, 0x2, 0x2, 0x76, 0x77, 0x7, 0x26, 
       0x2, 0x2, 0x77, 0x79, 0x7, 0x31, 0x2, 0x2, 0x78, 0x76, 0x3, 0x2, 
       0x2, 0x2, 0x78, 0x79, 0x3, 0x2, 0x2, 0x2, 0x79, 0x7a, 0x3, 0x2, 0x2, 
       0x2, 0x7a, 0x7b, 0x7, 0x3c, 0x2, 0x2, 0x7b, 0x19, 0x3, 0x2, 0x2, 
       0x2, 0x7c, 0x7d, 0x8, 0xe, 0x1, 0x2, 0x7d, 0x7e, 0x5, 0x1c, 0xf, 
       0x2, 0x7e, 0x87, 0x3, 0x2, 0x2, 0x2, 0x7f, 0x80, 0xc, 0x5, 0x2, 0x2, 
       0x80, 0x81, 0x7, 0x3e, 0x2, 0x2, 0x81, 0x86, 0x5, 0x1a, 0xe, 0x6, 
       0x82, 0x83, 0xc, 0x4, 0x2, 0x2, 0x83, 0x84, 0x7, 0x3f, 0x2, 0x2, 
       0x84, 0x86, 0x5, 0x1a, 0xe, 0x5, 0x85, 0x7f, 0x3, 0x2, 0x2, 0x2, 
       0x85, 0x82, 0x3, 0x2, 0x2, 0x2, 0x86, 0x89, 0x3, 0x2, 0x2, 0x2, 0x87, 
       0x85, 0x3, 0x2, 0x2, 0x2, 0x87, 0x88, 0x3, 0x2, 0x2, 0x2, 0x88, 0x1b, 
       0x3, 0x2, 0x2, 0x2, 0x89, 0x87, 0x3, 0x2, 0x2, 0x2, 0x8a, 0x8b, 0x8, 
       0xf, 0x1, 0x2, 0x8b, 0x8c, 0x7, 0x5, 0x2, 0x2, 0x8c, 0x8d, 0x5, 0x1a, 
       0xe, 0x2, 0x8d, 0x8e, 0x7, 0x6, 0x2, 0x2, 0x8e, 0x91, 0x3, 0x2, 0x2, 
       0x2, 0x8f, 0x91, 0x5, 0x1e, 0x10, 0x2, 0x90, 0x8a, 0x3, 0x2, 0x2, 
       0x2, 0x90, 0x8f, 0x3, 0x2, 0x2, 0x2, 0x91, 0x9a, 0x3, 0x2, 0x2, 0x2, 
       0x92, 0x93, 0xc, 0x6, 0x2, 0x2, 0x93, 0x94, 0x7, 0x3c, 0x2, 0x2, 
       0x94, 0x99, 0x5, 0x1c, 0xf, 0x7, 0x95, 0x96, 0xc, 0x5, 0x2, 0x2, 
       0x96, 0x97, 0x7, 0x3d, 0x2, 0x2, 0x97, 0x99, 0x5, 0x1c, 0xf, 0x6, 
       0x98, 0x92, 0x3, 0x2, 0x2, 0x2, 0x98, 0x95, 0x3, 0x2, 0x2, 0x2, 0x99, 
       0x9c, 0x3, 0x2, 0x2, 0x2, 0x9a, 0x98, 0x3, 0x2, 0x2, 0x2, 0x9a, 0x9b, 
       0x3, 0x2, 0x2, 0x2, 0x9b, 0x1d, 0x3, 0x2, 0x2, 0x2, 0x9c, 0x9a, 0x3, 
       0x2, 0x2, 0x2, 0x9d, 0xa5, 0x7, 0x28, 0x2, 0x2, 0x9e, 0xa5, 0x7, 
       0x2b, 0x2, 0x2, 0x9f, 0xa5, 0x7, 0x29, 0x2, 0x2, 0xa0, 0xa5, 0x5, 
       0x16, 0xc, 0x2, 0xa1, 0xa5, 0x5, 0x28, 0x15, 0x2, 0xa2, 0xa5, 0x5, 
       0x14, 0xb, 0x2, 0xa3, 0xa5, 0x5, 0x26, 0x14, 0x2, 0xa4, 0x9d, 0x3, 
       0x2, 0x2, 0x2, 0xa4, 0x9e, 0x3, 0x2, 0x2, 0x2, 0xa4, 0x9f, 0x3, 0x2, 
       0x2, 0x2, 0xa4, 0xa0, 0x3, 0x2, 0x2, 0x2, 0xa4, 0xa1, 0x3, 0x2, 0x2, 
       0x2, 0xa4, 0xa2, 0x3, 0x2, 0x2, 0x2, 0xa4, 0xa3, 0x3, 0x2, 0x2, 0x2, 
       0xa5, 0x1f, 0x3, 0x2, 0x2, 0x2, 0xa6, 0xaf, 0x5, 0x22, 0x12, 0x2, 
       0xa7, 0xa8, 0x7, 0x2d, 0x2, 0x2, 0xa8, 0xae, 0x7, 0x29, 0x2, 0x2, 
       0xa9, 0xaa, 0x7, 0x3e, 0x2, 0x2, 0xaa, 0xae, 0x5, 0x22, 0x12, 0x2, 
       0xab, 0xac, 0x7, 0x3f, 0x2, 0x2, 0xac, 0xae, 0x5, 0x6, 0x4, 0x2, 
       0xad, 0xa7, 0x3, 0x2, 0x2, 0x2, 0xad, 0xa9, 0x3, 0x2, 0x2, 0x2, 0xad, 
       0xab, 0x3, 0x2, 0x2, 0x2, 0xae, 0xb1, 0x3, 0x2, 0x2, 0x2, 0xaf, 0xad, 
       0x3, 0x2, 0x2, 0x2, 0xaf, 0xb0, 0x3, 0x2, 0x2, 0x2, 0xb0, 0x21, 0x3, 
       0x2, 0x2, 0x2, 0xb1, 0xaf, 0x3, 0x2, 0x2, 0x2, 0xb2, 0xc6, 0x5, 0x24, 
       0x13, 0x2, 0xb3, 0xb4, 0x7, 0x34, 0x2, 0x2, 0xb4, 0xc5, 0x7, 0x26, 
       0x2, 0x2, 0xb5, 0xb6, 0x7, 0x35, 0x2, 0x2, 0xb6, 0xc5, 0x5, 0x6, 
       0x4, 0x2, 0xb7, 0xb8, 0x7, 0x36, 0x2, 0x2, 0xb8, 0xc5, 0x5, 0x6, 
       0x4, 0x2, 0xb9, 0xba, 0x7, 0x33, 0x2, 0x2, 0xba, 0xbb, 0x7, 0x5, 
       0x2, 0x2, 0xbb, 0xbc, 0x7, 0x29, 0x2, 0x2, 0xbc, 0xbe, 0x7, 0x38, 
       0x2, 0x2, 0xbd, 0xbf, 0x9, 0x5, 0x2, 0x2, 0xbe, 0xbd, 0x3, 0x2, 0x2, 
       0x2, 0xbe, 0xbf, 0x3, 0x2, 0x2, 0x2, 0xbf, 0xc0, 0x3, 0x2, 0x2, 0x2, 
       0xc0, 0xc1, 0x7, 0x29, 0x2, 0x2, 0xc1, 0xc5, 0x7, 0x6, 0x2, 0x2, 
       0xc2, 0xc3, 0x7, 0x31, 0x2, 0x2, 0xc3, 0xc5, 0x5, 0x26, 0x14, 0x2, 
       0xc4, 0xb3, 0x3, 0x2, 0x2, 0x2, 0xc4, 0xb5, 0x3, 0x2, 0x2, 0x2, 0xc4, 
       0xb7, 0x3, 0x2, 0x2, 0x2, 0xc4, 0xb9, 0x3, 0x2, 0x2, 0x2, 0xc4, 0xc2, 
       0x3, 0x2, 0x2, 0x2, 0xc5, 0xc8, 0x3, 0x2, 0x2, 0x2, 0xc6, 0xc4, 0x3, 
       0x2, 0x2, 0x2, 0xc6, 0xc7, 0x3, 0x2, 0x2, 0x2, 0xc7, 0x23, 0x3, 0x2, 
       0x2, 0x2, 0xc8, 0xc6, 0x3, 0x2, 0x2, 0x2, 0xc9, 0xcf, 0x7, 0x26, 
       0x2, 0x2, 0xca, 0xcb, 0x7, 0x5, 0x2, 0x2, 0xcb, 0xcc, 0x5, 0x20, 
       0x11, 0x2, 0xcc, 0xcd, 0x7, 0x6, 0x2, 0x2, 0xcd, 0xcf, 0x3, 0x2, 
       0x2, 0x2, 0xce, 0xc9, 0x3, 0x2, 0x2, 0x2, 0xce, 0xca, 0x3, 0x2, 0x2, 
       0x2, 0xcf, 0x25, 0x3, 0x2, 0x2, 0x2, 0xd0, 0xd1, 0x9, 0x6, 0x2, 0x2, 
       0xd1, 0x27, 0x3, 0x2, 0x2, 0x2, 0xd2, 0xd3, 0x9, 0x7, 0x2, 0x2, 0xd3, 
       0xd4, 0x7, 0x5, 0x2, 0x2, 0xd4, 0xd5, 0x5, 0x1a, 0xe, 0x2, 0xd5, 
       0xd6, 0x7, 0x6, 0x2, 0x2, 0xd6, 0x29, 0x3, 0x2, 0x2, 0x2, 0x17, 0x2c, 
       0x2e, 0x49, 0x54, 0x5b, 0x60, 0x6e, 0x74, 0x78, 0x85, 0x87, 0x90, 
       0x98, 0x9a, 0xa4, 0xad, 0xaf, 0xbe, 0xc4, 0xc6, 0xce, 
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
