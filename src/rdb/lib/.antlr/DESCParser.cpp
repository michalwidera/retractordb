
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
      "'INTEGER'", "'FLOAT'", "'DOUBLE'", "'RATIONAL'", "'INTPAIR'", "'IDXPAIR'", 
      "'TYPE'", "'REF'", "'RETENTION'", "'RETMEMORY'", "'.'", "'-'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "BYTE_T", "STRING_T", "UNSIGNED_T", "INTEGER_T", 
      "FLOAT_T", "DOUBLE_T", "RATIONAL_T", "INTPAIR_T", "IDXPAIR_T", "TYPE_T", 
      "REF_T", "RETENTION_T", "RETMEMORY_T", "DOT", "MINUS", "ID", "STRING", 
      "DECIMAL", "FILENAME", "SPACE"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,25,74,2,0,7,0,2,1,7,1,1,0,1,0,5,0,7,8,0,10,0,12,0,10,9,0,1,0,1,0,
  	1,1,1,1,1,1,1,1,1,1,3,1,19,8,1,1,1,1,1,1,1,1,1,1,1,3,1,26,8,1,1,1,1,1,
  	1,1,1,1,1,1,3,1,33,8,1,1,1,1,1,1,1,1,1,1,1,3,1,40,8,1,1,1,1,1,1,1,1,1,
  	1,1,3,1,47,8,1,1,1,1,1,1,1,1,1,1,1,3,1,54,8,1,1,1,1,1,1,1,1,1,1,1,1,1,
  	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,72,8,1,1,1,0,0,2,0,2,0,0,
  	88,0,4,1,0,0,0,2,71,1,0,0,0,4,8,5,1,0,0,5,7,3,2,1,0,6,5,1,0,0,0,7,10,
  	1,0,0,0,8,6,1,0,0,0,8,9,1,0,0,0,9,11,1,0,0,0,10,8,1,0,0,0,11,12,5,2,0,
  	0,12,1,1,0,0,0,13,14,5,6,0,0,14,18,5,21,0,0,15,16,5,3,0,0,16,17,5,23,
  	0,0,17,19,5,4,0,0,18,15,1,0,0,0,18,19,1,0,0,0,19,72,1,0,0,0,20,21,5,9,
  	0,0,21,25,5,21,0,0,22,23,5,3,0,0,23,24,5,23,0,0,24,26,5,4,0,0,25,22,1,
  	0,0,0,25,26,1,0,0,0,26,72,1,0,0,0,27,28,5,8,0,0,28,32,5,21,0,0,29,30,
  	5,3,0,0,30,31,5,23,0,0,31,33,5,4,0,0,32,29,1,0,0,0,32,33,1,0,0,0,33,72,
  	1,0,0,0,34,35,5,10,0,0,35,39,5,21,0,0,36,37,5,3,0,0,37,38,5,23,0,0,38,
  	40,5,4,0,0,39,36,1,0,0,0,39,40,1,0,0,0,40,72,1,0,0,0,41,42,5,11,0,0,42,
  	46,5,21,0,0,43,44,5,3,0,0,44,45,5,23,0,0,45,47,5,4,0,0,46,43,1,0,0,0,
  	46,47,1,0,0,0,47,72,1,0,0,0,48,49,5,12,0,0,49,53,5,21,0,0,50,51,5,3,0,
  	0,51,52,5,23,0,0,52,54,5,4,0,0,53,50,1,0,0,0,53,54,1,0,0,0,54,72,1,0,
  	0,0,55,56,5,16,0,0,56,57,5,5,0,0,57,58,5,24,0,0,58,72,5,5,0,0,59,60,5,
  	15,0,0,60,72,5,21,0,0,61,62,5,7,0,0,62,63,5,21,0,0,63,64,5,3,0,0,64,65,
  	5,23,0,0,65,72,5,4,0,0,66,67,5,17,0,0,67,68,5,23,0,0,68,72,5,23,0,0,69,
  	70,5,18,0,0,70,72,5,23,0,0,71,13,1,0,0,0,71,20,1,0,0,0,71,27,1,0,0,0,
  	71,34,1,0,0,0,71,41,1,0,0,0,71,48,1,0,0,0,71,55,1,0,0,0,71,59,1,0,0,0,
  	71,61,1,0,0,0,71,66,1,0,0,0,71,69,1,0,0,0,72,3,1,0,0,0,8,8,18,25,32,39,
  	46,53,71
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
      ((1ULL << _la) & 499648) != 0)) {
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

//----------------- UnsignedIDContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::UnsignedIDContext::UNSIGNED_T() {
  return getToken(DESCParser::UNSIGNED_T, 0);
}

tree::TerminalNode* DESCParser::UnsignedIDContext::ID() {
  return getToken(DESCParser::ID, 0);
}

tree::TerminalNode* DESCParser::UnsignedIDContext::DECIMAL() {
  return getToken(DESCParser::DECIMAL, 0);
}

DESCParser::UnsignedIDContext::UnsignedIDContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::UnsignedIDContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnsignedID(this);
}
void DESCParser::UnsignedIDContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnsignedID(this);
}
//----------------- TypeIDContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::TypeIDContext::TYPE_T() {
  return getToken(DESCParser::TYPE_T, 0);
}

tree::TerminalNode* DESCParser::TypeIDContext::ID() {
  return getToken(DESCParser::ID, 0);
}

DESCParser::TypeIDContext::TypeIDContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::TypeIDContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTypeID(this);
}
void DESCParser::TypeIDContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTypeID(this);
}
//----------------- RetMemoryIDContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::RetMemoryIDContext::RETMEMORY_T() {
  return getToken(DESCParser::RETMEMORY_T, 0);
}

tree::TerminalNode* DESCParser::RetMemoryIDContext::DECIMAL() {
  return getToken(DESCParser::DECIMAL, 0);
}

DESCParser::RetMemoryIDContext::RetMemoryIDContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::RetMemoryIDContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRetMemoryID(this);
}
void DESCParser::RetMemoryIDContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRetMemoryID(this);
}
//----------------- StringIDContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::StringIDContext::STRING_T() {
  return getToken(DESCParser::STRING_T, 0);
}

tree::TerminalNode* DESCParser::StringIDContext::ID() {
  return getToken(DESCParser::ID, 0);
}

tree::TerminalNode* DESCParser::StringIDContext::DECIMAL() {
  return getToken(DESCParser::DECIMAL, 0);
}

DESCParser::StringIDContext::StringIDContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::StringIDContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStringID(this);
}
void DESCParser::StringIDContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStringID(this);
}
//----------------- IntegerIDContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::IntegerIDContext::INTEGER_T() {
  return getToken(DESCParser::INTEGER_T, 0);
}

tree::TerminalNode* DESCParser::IntegerIDContext::ID() {
  return getToken(DESCParser::ID, 0);
}

tree::TerminalNode* DESCParser::IntegerIDContext::DECIMAL() {
  return getToken(DESCParser::DECIMAL, 0);
}

DESCParser::IntegerIDContext::IntegerIDContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::IntegerIDContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterIntegerID(this);
}
void DESCParser::IntegerIDContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitIntegerID(this);
}
//----------------- RetentionIDContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::RetentionIDContext::RETENTION_T() {
  return getToken(DESCParser::RETENTION_T, 0);
}

std::vector<tree::TerminalNode *> DESCParser::RetentionIDContext::DECIMAL() {
  return getTokens(DESCParser::DECIMAL);
}

tree::TerminalNode* DESCParser::RetentionIDContext::DECIMAL(size_t i) {
  return getToken(DESCParser::DECIMAL, i);
}

DESCParser::RetentionIDContext::RetentionIDContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::RetentionIDContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRetentionID(this);
}
void DESCParser::RetentionIDContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRetentionID(this);
}
//----------------- RationalIDContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::RationalIDContext::RATIONAL_T() {
  return getToken(DESCParser::RATIONAL_T, 0);
}

tree::TerminalNode* DESCParser::RationalIDContext::ID() {
  return getToken(DESCParser::ID, 0);
}

tree::TerminalNode* DESCParser::RationalIDContext::DECIMAL() {
  return getToken(DESCParser::DECIMAL, 0);
}

DESCParser::RationalIDContext::RationalIDContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::RationalIDContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRationalID(this);
}
void DESCParser::RationalIDContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRationalID(this);
}
//----------------- ByteIDContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::ByteIDContext::BYTE_T() {
  return getToken(DESCParser::BYTE_T, 0);
}

tree::TerminalNode* DESCParser::ByteIDContext::ID() {
  return getToken(DESCParser::ID, 0);
}

tree::TerminalNode* DESCParser::ByteIDContext::DECIMAL() {
  return getToken(DESCParser::DECIMAL, 0);
}

DESCParser::ByteIDContext::ByteIDContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::ByteIDContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterByteID(this);
}
void DESCParser::ByteIDContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitByteID(this);
}
//----------------- RefIDContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::RefIDContext::REF_T() {
  return getToken(DESCParser::REF_T, 0);
}

tree::TerminalNode* DESCParser::RefIDContext::FILENAME() {
  return getToken(DESCParser::FILENAME, 0);
}

DESCParser::RefIDContext::RefIDContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::RefIDContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRefID(this);
}
void DESCParser::RefIDContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRefID(this);
}
//----------------- FloatIDContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::FloatIDContext::FLOAT_T() {
  return getToken(DESCParser::FLOAT_T, 0);
}

tree::TerminalNode* DESCParser::FloatIDContext::ID() {
  return getToken(DESCParser::ID, 0);
}

tree::TerminalNode* DESCParser::FloatIDContext::DECIMAL() {
  return getToken(DESCParser::DECIMAL, 0);
}

DESCParser::FloatIDContext::FloatIDContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::FloatIDContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFloatID(this);
}
void DESCParser::FloatIDContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFloatID(this);
}
//----------------- DoubleIDContext ------------------------------------------------------------------

tree::TerminalNode* DESCParser::DoubleIDContext::DOUBLE_T() {
  return getToken(DESCParser::DOUBLE_T, 0);
}

tree::TerminalNode* DESCParser::DoubleIDContext::ID() {
  return getToken(DESCParser::ID, 0);
}

tree::TerminalNode* DESCParser::DoubleIDContext::DECIMAL() {
  return getToken(DESCParser::DECIMAL, 0);
}

DESCParser::DoubleIDContext::DoubleIDContext(CommandContext *ctx) { copyFrom(ctx); }

void DESCParser::DoubleIDContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDoubleID(this);
}
void DESCParser::DoubleIDContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<DESCListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDoubleID(this);
}
DESCParser::CommandContext* DESCParser::command() {
  CommandContext *_localctx = _tracker.createInstance<CommandContext>(_ctx, getState());
  enterRule(_localctx, 2, DESCParser::RuleCommand);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(71);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case DESCParser::BYTE_T: {
        _localctx = _tracker.createInstance<DESCParser::ByteIDContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(13);
        match(DESCParser::BYTE_T);
        setState(14);
        antlrcpp::downCast<ByteIDContext *>(_localctx)->name = match(DESCParser::ID);
        setState(18);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == DESCParser::T__2) {
          setState(15);
          match(DESCParser::T__2);
          setState(16);
          antlrcpp::downCast<ByteIDContext *>(_localctx)->arr = match(DESCParser::DECIMAL);
          setState(17);
          match(DESCParser::T__3);
        }
        break;
      }

      case DESCParser::INTEGER_T: {
        _localctx = _tracker.createInstance<DESCParser::IntegerIDContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(20);
        match(DESCParser::INTEGER_T);
        setState(21);
        antlrcpp::downCast<IntegerIDContext *>(_localctx)->name = match(DESCParser::ID);
        setState(25);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == DESCParser::T__2) {
          setState(22);
          match(DESCParser::T__2);
          setState(23);
          antlrcpp::downCast<IntegerIDContext *>(_localctx)->arr = match(DESCParser::DECIMAL);
          setState(24);
          match(DESCParser::T__3);
        }
        break;
      }

      case DESCParser::UNSIGNED_T: {
        _localctx = _tracker.createInstance<DESCParser::UnsignedIDContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(27);
        match(DESCParser::UNSIGNED_T);
        setState(28);
        antlrcpp::downCast<UnsignedIDContext *>(_localctx)->name = match(DESCParser::ID);
        setState(32);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == DESCParser::T__2) {
          setState(29);
          match(DESCParser::T__2);
          setState(30);
          antlrcpp::downCast<UnsignedIDContext *>(_localctx)->arr = match(DESCParser::DECIMAL);
          setState(31);
          match(DESCParser::T__3);
        }
        break;
      }

      case DESCParser::FLOAT_T: {
        _localctx = _tracker.createInstance<DESCParser::FloatIDContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(34);
        match(DESCParser::FLOAT_T);
        setState(35);
        antlrcpp::downCast<FloatIDContext *>(_localctx)->name = match(DESCParser::ID);
        setState(39);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == DESCParser::T__2) {
          setState(36);
          match(DESCParser::T__2);
          setState(37);
          antlrcpp::downCast<FloatIDContext *>(_localctx)->arr = match(DESCParser::DECIMAL);
          setState(38);
          match(DESCParser::T__3);
        }
        break;
      }

      case DESCParser::DOUBLE_T: {
        _localctx = _tracker.createInstance<DESCParser::DoubleIDContext>(_localctx);
        enterOuterAlt(_localctx, 5);
        setState(41);
        match(DESCParser::DOUBLE_T);
        setState(42);
        antlrcpp::downCast<DoubleIDContext *>(_localctx)->name = match(DESCParser::ID);
        setState(46);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == DESCParser::T__2) {
          setState(43);
          match(DESCParser::T__2);
          setState(44);
          antlrcpp::downCast<DoubleIDContext *>(_localctx)->arr = match(DESCParser::DECIMAL);
          setState(45);
          match(DESCParser::T__3);
        }
        break;
      }

      case DESCParser::RATIONAL_T: {
        _localctx = _tracker.createInstance<DESCParser::RationalIDContext>(_localctx);
        enterOuterAlt(_localctx, 6);
        setState(48);
        match(DESCParser::RATIONAL_T);
        setState(49);
        antlrcpp::downCast<RationalIDContext *>(_localctx)->name = match(DESCParser::ID);
        setState(53);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == DESCParser::T__2) {
          setState(50);
          match(DESCParser::T__2);
          setState(51);
          antlrcpp::downCast<RationalIDContext *>(_localctx)->arr = match(DESCParser::DECIMAL);
          setState(52);
          match(DESCParser::T__3);
        }
        break;
      }

      case DESCParser::REF_T: {
        _localctx = _tracker.createInstance<DESCParser::RefIDContext>(_localctx);
        enterOuterAlt(_localctx, 7);
        setState(55);
        match(DESCParser::REF_T);
        setState(56);
        match(DESCParser::T__4);
        setState(57);
        antlrcpp::downCast<RefIDContext *>(_localctx)->file = match(DESCParser::FILENAME);
        setState(58);
        match(DESCParser::T__4);
        break;
      }

      case DESCParser::TYPE_T: {
        _localctx = _tracker.createInstance<DESCParser::TypeIDContext>(_localctx);
        enterOuterAlt(_localctx, 8);
        setState(59);
        match(DESCParser::TYPE_T);
        setState(60);
        antlrcpp::downCast<TypeIDContext *>(_localctx)->type = match(DESCParser::ID);
        break;
      }

      case DESCParser::STRING_T: {
        _localctx = _tracker.createInstance<DESCParser::StringIDContext>(_localctx);
        enterOuterAlt(_localctx, 9);
        setState(61);
        match(DESCParser::STRING_T);
        setState(62);
        antlrcpp::downCast<StringIDContext *>(_localctx)->name = match(DESCParser::ID);
        setState(63);
        match(DESCParser::T__2);
        setState(64);
        antlrcpp::downCast<StringIDContext *>(_localctx)->strsize = match(DESCParser::DECIMAL);
        setState(65);
        match(DESCParser::T__3);
        break;
      }

      case DESCParser::RETENTION_T: {
        _localctx = _tracker.createInstance<DESCParser::RetentionIDContext>(_localctx);
        enterOuterAlt(_localctx, 10);
        setState(66);
        match(DESCParser::RETENTION_T);
        setState(67);
        antlrcpp::downCast<RetentionIDContext *>(_localctx)->capacity = match(DESCParser::DECIMAL);
        setState(68);
        antlrcpp::downCast<RetentionIDContext *>(_localctx)->segment = match(DESCParser::DECIMAL);
        break;
      }

      case DESCParser::RETMEMORY_T: {
        _localctx = _tracker.createInstance<DESCParser::RetMemoryIDContext>(_localctx);
        enterOuterAlt(_localctx, 11);
        setState(69);
        match(DESCParser::RETMEMORY_T);
        setState(70);
        antlrcpp::downCast<RetMemoryIDContext *>(_localctx)->capacity = match(DESCParser::DECIMAL);
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
