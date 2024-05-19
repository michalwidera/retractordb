
// Generated from DESC.g4 by ANTLR 4.13.1


#include "DESCListener.h"

#include "DESCParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct DESCParserStaticData final {
  DESCParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  DESCParserStaticData(const DESCParserStaticData&) = delete;
  DESCParserStaticData(DESCParserStaticData&&) = delete;
  DESCParserStaticData& operator=(const DESCParserStaticData&) = delete;
  DESCParserStaticData& operator=(DESCParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag descParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
DESCParserStaticData *descParserStaticData = nullptr;

void descParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (descParserStaticData != nullptr) {
    return;
  }
#else
  assert(descParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<DESCParserStaticData>(
    std::vector<std::string>{
      "desc", "command"
    },
    std::vector<std::string>{
      "", "'{'", "'}'", "'['", "']'", "'\"'", "'BYTE'", "'STRING'", "'UINT'", 
      "'INTEGER'", "'FLOAT'", "'DOUBLE'", "'RATIONAl'", "'INTPAIR'", "'IDXPAIR'", 
      "'TYPE'", "'REF'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "BYTE_T", "STRING_T", "UNSIGNED_T", "INTEGER_T", 
      "FLOAT_T", "DOUBLE_T", "RATIONAL_T", "INTPAIR_T", "IDXPAIR_T", "TYPE_T", 
      "REF_T", "ID", "STRING", "DECIMAL", "REF_TYPE_ARG"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,20,36,2,0,7,0,2,1,7,1,1,0,1,0,5,0,7,8,0,10,0,12,0,10,9,0,1,0,1,0,
  	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  	1,1,1,1,1,3,1,34,8,1,1,1,0,0,2,0,2,0,0,40,0,4,1,0,0,0,2,33,1,0,0,0,4,
  	8,5,1,0,0,5,7,3,2,1,0,6,5,1,0,0,0,7,10,1,0,0,0,8,6,1,0,0,0,8,9,1,0,0,
  	0,9,11,1,0,0,0,10,8,1,0,0,0,11,12,5,2,0,0,12,1,1,0,0,0,13,14,5,6,0,0,
  	14,34,5,17,0,0,15,16,5,7,0,0,16,17,5,17,0,0,17,18,5,3,0,0,18,19,5,19,
  	0,0,19,34,5,4,0,0,20,21,5,8,0,0,21,34,5,17,0,0,22,23,5,10,0,0,23,34,5,
  	17,0,0,24,25,5,11,0,0,25,34,5,17,0,0,26,27,5,16,0,0,27,28,5,18,0,0,28,
  	29,5,5,0,0,29,30,5,18,0,0,30,34,5,5,0,0,31,32,5,15,0,0,32,34,5,20,0,0,
  	33,13,1,0,0,0,33,15,1,0,0,0,33,20,1,0,0,0,33,22,1,0,0,0,33,24,1,0,0,0,
  	33,26,1,0,0,0,33,31,1,0,0,0,34,3,1,0,0,0,2,8,33
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  descParserStaticData = staticData.release();
}

}

DESCParser::DESCParser(TokenStream *input) : DESCParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

DESCParser::DESCParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  DESCParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *descParserStaticData->atn, descParserStaticData->decisionToDFA, descParserStaticData->sharedContextCache, options);
}

DESCParser::~DESCParser() {
  delete _interpreter;
}

const atn::ATN& DESCParser::getATN() const {
  return *descParserStaticData->atn;
}

std::string DESCParser::getGrammarFileName() const {
  return "DESC.g4";
}

const std::vector<std::string>& DESCParser::getRuleNames() const {
  return descParserStaticData->ruleNames;
}

const dfa::Vocabulary& DESCParser::getVocabulary() const {
  return descParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView DESCParser::getSerializedATN() const {
  return descParserStaticData->serializedATN;
}


//----------------- DescContext ------------------------------------------------------------------

DESCParser::DescContext::DescContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<DESCParser::CommandContext *> DESCParser::DescContext::command() {
  return getRuleContexts<DESCParser::CommandContext>();
}

DESCParser::CommandContext* DESCParser::DescContext::command(size_t i) {
  return getRuleContext<DESCParser::CommandContext>(i);
}


size_t DESCParser::DescContext::getRuleIndex() const {
  return DESCParser::RuleDesc;
}

void DESCParser::DescContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDesc(this);
}

void DESCParser::DescContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDesc(this);
}

DESCParser::DescContext* DESCParser::desc() {
  DescContext *_localctx = _tracker.createInstance<DescContext>(_ctx, getState());
  enterRule(_localctx, 0, DESCParser::RuleDesc);
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
    setState(4);
    match(DESCParser::T__0);
    setState(8);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 101824) != 0)) {
      setState(5);
      command();
      setState(10);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(11);
    match(DESCParser::T__1);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- CommandContext ------------------------------------------------------------------

DESCParser::CommandContext::CommandContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t DESCParser::CommandContext::getRuleIndex() const {
  return DESCParser::RuleCommand;
}

void DESCParser::CommandContext::copyFrom(CommandContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- FloatContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::FloatContext::FLOAT_T() {
  return getToken(DESCParser::FLOAT_T, 0);
}

tree::TerminalNode* DESCParser::FloatContext::ID() {
  return getToken(DESCParser::ID, 0);
}

DESCParser::FloatContext::FloatContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::FloatContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFloat(this);
}
void DESCParser::FloatContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFloat(this);
}
//----------------- RefContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::RefContext::REF_T() {
  return getToken(DESCParser::REF_T, 0);
}

std::vector<tree::TerminalNode *> DESCParser::RefContext::STRING() {
  return getTokens(DESCParser::STRING);
}

tree::TerminalNode* DESCParser::RefContext::STRING(size_t i) {
  return getToken(DESCParser::STRING, i);
}

DESCParser::RefContext::RefContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::RefContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRef(this);
}
void DESCParser::RefContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRef(this);
}
//----------------- TypeContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::TypeContext::TYPE_T() {
  return getToken(DESCParser::TYPE_T, 0);
}

tree::TerminalNode* DESCParser::TypeContext::REF_TYPE_ARG() {
  return getToken(DESCParser::REF_TYPE_ARG, 0);
}

DESCParser::TypeContext::TypeContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::TypeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterType(this);
}
void DESCParser::TypeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitType(this);
}
//----------------- UnsignedContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::UnsignedContext::UNSIGNED_T() {
  return getToken(DESCParser::UNSIGNED_T, 0);
}

tree::TerminalNode* DESCParser::UnsignedContext::ID() {
  return getToken(DESCParser::ID, 0);
}

DESCParser::UnsignedContext::UnsignedContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::UnsignedContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnsigned(this);
}
void DESCParser::UnsignedContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnsigned(this);
}
//----------------- ByteContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::ByteContext::BYTE_T() {
  return getToken(DESCParser::BYTE_T, 0);
}

tree::TerminalNode* DESCParser::ByteContext::ID() {
  return getToken(DESCParser::ID, 0);
}

DESCParser::ByteContext::ByteContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::ByteContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterByte(this);
}
void DESCParser::ByteContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitByte(this);
}
//----------------- StringContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::StringContext::STRING_T() {
  return getToken(DESCParser::STRING_T, 0);
}

tree::TerminalNode* DESCParser::StringContext::DECIMAL() {
  return getToken(DESCParser::DECIMAL, 0);
}

tree::TerminalNode* DESCParser::StringContext::ID() {
  return getToken(DESCParser::ID, 0);
}

DESCParser::StringContext::StringContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::StringContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterString(this);
}
void DESCParser::StringContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitString(this);
}
//----------------- DoubleContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::DoubleContext::DOUBLE_T() {
  return getToken(DESCParser::DOUBLE_T, 0);
}

tree::TerminalNode* DESCParser::DoubleContext::ID() {
  return getToken(DESCParser::ID, 0);
}

DESCParser::DoubleContext::DoubleContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::DoubleContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDouble(this);
}
void DESCParser::DoubleContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDouble(this);
}
DESCParser::CommandContext* DESCParser::command() {
  CommandContext *_localctx = _tracker.createInstance<CommandContext>(_ctx, getState());
  enterRule(_localctx, 2, DESCParser::RuleCommand);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(33);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case DESCParser::BYTE_T: {
        _localctx = _tracker.createInstance<DESCParser::ByteContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(13);
        match(DESCParser::BYTE_T);
        setState(14);
        antlrcpp::downCast<ByteContext *>(_localctx)->name = match(DESCParser::ID);
        break;
      }

      case DESCParser::STRING_T: {
        _localctx = _tracker.createInstance<DESCParser::StringContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(15);
        match(DESCParser::STRING_T);
        setState(16);
        antlrcpp::downCast<StringContext *>(_localctx)->name = match(DESCParser::ID);
        setState(17);
        match(DESCParser::T__2);
        setState(18);
        match(DESCParser::DECIMAL);
        setState(19);
        match(DESCParser::T__3);
        break;
      }

      case DESCParser::UNSIGNED_T: {
        _localctx = _tracker.createInstance<DESCParser::UnsignedContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(20);
        match(DESCParser::UNSIGNED_T);
        setState(21);
        antlrcpp::downCast<UnsignedContext *>(_localctx)->name = match(DESCParser::ID);
        break;
      }

      case DESCParser::FLOAT_T: {
        _localctx = _tracker.createInstance<DESCParser::FloatContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(22);
        match(DESCParser::FLOAT_T);
        setState(23);
        antlrcpp::downCast<FloatContext *>(_localctx)->name = match(DESCParser::ID);
        break;
      }

      case DESCParser::DOUBLE_T: {
        _localctx = _tracker.createInstance<DESCParser::DoubleContext>(_localctx);
        enterOuterAlt(_localctx, 5);
        setState(24);
        match(DESCParser::DOUBLE_T);
        setState(25);
        antlrcpp::downCast<DoubleContext *>(_localctx)->name = match(DESCParser::ID);
        break;
      }

      case DESCParser::REF_T: {
        _localctx = _tracker.createInstance<DESCParser::RefContext>(_localctx);
        enterOuterAlt(_localctx, 6);
        setState(26);
        match(DESCParser::REF_T);
        setState(27);
        antlrcpp::downCast<RefContext *>(_localctx)->name = match(DESCParser::STRING);
        setState(28);
        match(DESCParser::T__4);
        setState(29);
        antlrcpp::downCast<RefContext *>(_localctx)->file = match(DESCParser::STRING);
        setState(30);
        match(DESCParser::T__4);
        break;
      }

      case DESCParser::TYPE_T: {
        _localctx = _tracker.createInstance<DESCParser::TypeContext>(_localctx);
        enterOuterAlt(_localctx, 7);
        setState(31);
        match(DESCParser::TYPE_T);
        setState(32);
        antlrcpp::downCast<TypeContext *>(_localctx)->type = match(DESCParser::REF_TYPE_ARG);
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

void DESCParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  descParserInitialize();
#else
  ::antlr4::internal::call_once(descParserOnceFlag, descParserInitialize);
#endif
}
