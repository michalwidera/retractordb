
// Generated from DESC.g4 by ANTLR 4.13.1


#include "DESCLexer.h"


using namespace antlr4;



using namespace antlr4;

namespace {

struct DESCLexerStaticData final {
  DESCLexerStaticData(std::vector<std::string> ruleNames,
                          std::vector<std::string> channelNames,
                          std::vector<std::string> modeNames,
                          std::vector<std::string> literalNames,
                          std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  DESCLexerStaticData(const DESCLexerStaticData&) = delete;
  DESCLexerStaticData(DESCLexerStaticData&&) = delete;
  DESCLexerStaticData& operator=(const DESCLexerStaticData&) = delete;
  DESCLexerStaticData& operator=(DESCLexerStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> channelNames;
  const std::vector<std::string> modeNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag desclexerLexerOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
DESCLexerStaticData *desclexerLexerStaticData = nullptr;

void desclexerLexerInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (desclexerLexerStaticData != nullptr) {
    return;
  }
#else
  assert(desclexerLexerStaticData == nullptr);
#endif
  auto staticData = std::make_unique<DESCLexerStaticData>(
    std::vector<std::string>{
      "T__0", "T__1", "T__2", "T__3", "T__4", "BYTE_T", "STRING_T", "UNSIGNED_T", 
      "INTEGER_T", "FLOAT_T", "DOUBLE_T", "RATIONAL_T", "INTPAIR_T", "IDXPAIR_T", 
      "DEC_DIGIT", "TYPE_T", "REF_T", "ID", "STRING", "DECIMAL", "REF_TYPE_ARG"
    },
    std::vector<std::string>{
      "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
    },
    std::vector<std::string>{
      "DEFAULT_MODE"
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
  	4,0,20,182,6,-1,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,
  	6,2,7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,
  	7,14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,1,0,
  	1,0,1,1,1,1,1,2,1,2,1,3,1,3,1,4,1,4,1,5,1,5,1,5,1,5,1,5,1,6,1,6,1,6,1,
  	6,1,6,1,6,1,6,1,7,1,7,1,7,1,7,1,7,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,9,
  	1,9,1,9,1,9,1,9,1,9,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,11,1,11,1,11,
  	1,11,1,11,1,11,1,11,1,11,1,11,1,12,1,12,1,12,1,12,1,12,1,12,1,12,1,12,
  	1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,14,1,14,1,15,1,15,1,15,1,15,
  	1,15,1,16,1,16,1,16,1,16,1,17,1,17,5,17,130,8,17,10,17,12,17,133,9,17,
  	1,18,1,18,1,18,1,18,5,18,139,8,18,10,18,12,18,142,9,18,1,18,1,18,1,19,
  	4,19,147,8,19,11,19,12,19,148,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,
  	1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,
  	1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,3,20,181,8,20,0,0,21,1,1,3,2,
  	5,3,7,4,9,5,11,6,13,7,15,8,17,9,19,10,21,11,23,12,25,13,27,14,29,0,31,
  	15,33,16,35,17,37,18,39,19,41,20,1,0,4,1,0,48,57,2,0,65,90,97,122,5,0,
  	36,36,48,57,65,90,95,95,97,122,1,0,39,39,187,0,1,1,0,0,0,0,3,1,0,0,0,
  	0,5,1,0,0,0,0,7,1,0,0,0,0,9,1,0,0,0,0,11,1,0,0,0,0,13,1,0,0,0,0,15,1,
  	0,0,0,0,17,1,0,0,0,0,19,1,0,0,0,0,21,1,0,0,0,0,23,1,0,0,0,0,25,1,0,0,
  	0,0,27,1,0,0,0,0,31,1,0,0,0,0,33,1,0,0,0,0,35,1,0,0,0,0,37,1,0,0,0,0,
  	39,1,0,0,0,0,41,1,0,0,0,1,43,1,0,0,0,3,45,1,0,0,0,5,47,1,0,0,0,7,49,1,
  	0,0,0,9,51,1,0,0,0,11,53,1,0,0,0,13,58,1,0,0,0,15,65,1,0,0,0,17,70,1,
  	0,0,0,19,78,1,0,0,0,21,84,1,0,0,0,23,91,1,0,0,0,25,100,1,0,0,0,27,108,
  	1,0,0,0,29,116,1,0,0,0,31,118,1,0,0,0,33,123,1,0,0,0,35,127,1,0,0,0,37,
  	134,1,0,0,0,39,146,1,0,0,0,41,180,1,0,0,0,43,44,5,123,0,0,44,2,1,0,0,
  	0,45,46,5,125,0,0,46,4,1,0,0,0,47,48,5,91,0,0,48,6,1,0,0,0,49,50,5,93,
  	0,0,50,8,1,0,0,0,51,52,5,34,0,0,52,10,1,0,0,0,53,54,5,66,0,0,54,55,5,
  	89,0,0,55,56,5,84,0,0,56,57,5,69,0,0,57,12,1,0,0,0,58,59,5,83,0,0,59,
  	60,5,84,0,0,60,61,5,82,0,0,61,62,5,73,0,0,62,63,5,78,0,0,63,64,5,71,0,
  	0,64,14,1,0,0,0,65,66,5,85,0,0,66,67,5,73,0,0,67,68,5,78,0,0,68,69,5,
  	84,0,0,69,16,1,0,0,0,70,71,5,73,0,0,71,72,5,78,0,0,72,73,5,84,0,0,73,
  	74,5,69,0,0,74,75,5,71,0,0,75,76,5,69,0,0,76,77,5,82,0,0,77,18,1,0,0,
  	0,78,79,5,70,0,0,79,80,5,76,0,0,80,81,5,79,0,0,81,82,5,65,0,0,82,83,5,
  	84,0,0,83,20,1,0,0,0,84,85,5,68,0,0,85,86,5,79,0,0,86,87,5,85,0,0,87,
  	88,5,66,0,0,88,89,5,76,0,0,89,90,5,69,0,0,90,22,1,0,0,0,91,92,5,82,0,
  	0,92,93,5,65,0,0,93,94,5,84,0,0,94,95,5,73,0,0,95,96,5,79,0,0,96,97,5,
  	78,0,0,97,98,5,65,0,0,98,99,5,108,0,0,99,24,1,0,0,0,100,101,5,73,0,0,
  	101,102,5,78,0,0,102,103,5,84,0,0,103,104,5,80,0,0,104,105,5,65,0,0,105,
  	106,5,73,0,0,106,107,5,82,0,0,107,26,1,0,0,0,108,109,5,73,0,0,109,110,
  	5,68,0,0,110,111,5,88,0,0,111,112,5,80,0,0,112,113,5,65,0,0,113,114,5,
  	73,0,0,114,115,5,82,0,0,115,28,1,0,0,0,116,117,7,0,0,0,117,30,1,0,0,0,
  	118,119,5,84,0,0,119,120,5,89,0,0,120,121,5,80,0,0,121,122,5,69,0,0,122,
  	32,1,0,0,0,123,124,5,82,0,0,124,125,5,69,0,0,125,126,5,70,0,0,126,34,
  	1,0,0,0,127,131,7,1,0,0,128,130,7,2,0,0,129,128,1,0,0,0,130,133,1,0,0,
  	0,131,129,1,0,0,0,131,132,1,0,0,0,132,36,1,0,0,0,133,131,1,0,0,0,134,
  	140,5,39,0,0,135,139,8,3,0,0,136,137,5,39,0,0,137,139,5,39,0,0,138,135,
  	1,0,0,0,138,136,1,0,0,0,139,142,1,0,0,0,140,138,1,0,0,0,140,141,1,0,0,
  	0,141,143,1,0,0,0,142,140,1,0,0,0,143,144,5,39,0,0,144,38,1,0,0,0,145,
  	147,3,29,14,0,146,145,1,0,0,0,147,148,1,0,0,0,148,146,1,0,0,0,148,149,
  	1,0,0,0,149,40,1,0,0,0,150,151,5,84,0,0,151,152,5,69,0,0,152,153,5,88,
  	0,0,153,154,5,84,0,0,154,155,5,83,0,0,155,156,5,79,0,0,156,157,5,85,0,
  	0,157,158,5,82,0,0,158,159,5,67,0,0,159,181,5,69,0,0,160,161,5,68,0,0,
  	161,162,5,69,0,0,162,163,5,86,0,0,163,164,5,73,0,0,164,165,5,67,0,0,165,
  	181,5,69,0,0,166,167,5,71,0,0,167,168,5,69,0,0,168,169,5,78,0,0,169,170,
  	5,69,0,0,170,171,5,82,0,0,171,172,5,73,0,0,172,181,5,67,0,0,173,174,5,
  	68,0,0,174,175,5,69,0,0,175,176,5,70,0,0,176,177,5,65,0,0,177,178,5,85,
  	0,0,178,179,5,76,0,0,179,181,5,84,0,0,180,150,1,0,0,0,180,160,1,0,0,0,
  	180,166,1,0,0,0,180,173,1,0,0,0,181,42,1,0,0,0,6,0,131,138,140,148,180,
  	0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  desclexerLexerStaticData = staticData.release();
}

}

DESCLexer::DESCLexer(CharStream *input) : Lexer(input) {
  DESCLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *desclexerLexerStaticData->atn, desclexerLexerStaticData->decisionToDFA, desclexerLexerStaticData->sharedContextCache);
}

DESCLexer::~DESCLexer() {
  delete _interpreter;
}

std::string DESCLexer::getGrammarFileName() const {
  return "DESC.g4";
}

const std::vector<std::string>& DESCLexer::getRuleNames() const {
  return desclexerLexerStaticData->ruleNames;
}

const std::vector<std::string>& DESCLexer::getChannelNames() const {
  return desclexerLexerStaticData->channelNames;
}

const std::vector<std::string>& DESCLexer::getModeNames() const {
  return desclexerLexerStaticData->modeNames;
}

const dfa::Vocabulary& DESCLexer::getVocabulary() const {
  return desclexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView DESCLexer::getSerializedATN() const {
  return desclexerLexerStaticData->serializedATN;
}

const atn::ATN& DESCLexer::getATN() const {
  return *desclexerLexerStaticData->atn;
}




void DESCLexer::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  desclexerLexerInitialize();
#else
  ::antlr4::internal::call_once(desclexerLexerOnceFlag, desclexerLexerInitialize);
#endif
}
