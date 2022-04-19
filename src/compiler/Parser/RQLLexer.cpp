
// Generated from RQL.g4 by ANTLR 4.10.1


#include "RQLLexer.h"


using namespace antlr4;



using namespace antlr4;

namespace {

struct RQLLexerStaticData final {
  RQLLexerStaticData(std::vector<std::string> ruleNames,
                          std::vector<std::string> channelNames,
                          std::vector<std::string> modeNames,
                          std::vector<std::string> literalNames,
                          std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  RQLLexerStaticData(const RQLLexerStaticData&) = delete;
  RQLLexerStaticData(RQLLexerStaticData&&) = delete;
  RQLLexerStaticData& operator=(const RQLLexerStaticData&) = delete;
  RQLLexerStaticData& operator=(RQLLexerStaticData&&) = delete;

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

std::once_flag rqllexerLexerOnceFlag;
RQLLexerStaticData *rqllexerLexerStaticData = nullptr;

void rqllexerLexerInitialize() {
  assert(rqllexerLexerStaticData == nullptr);
  auto staticData = std::make_unique<RQLLexerStaticData>(
    std::vector<std::string>{
      "T__0", "T__1", "T__2", "T__3", "T__4", "T__5", "T__6", "T__7", "T__8", 
      "T__9", "T__10", "T__11", "T__12", "T__13", "T__14", "T__15", "T__16", 
      "T__17", "T__18", "T__19", "STRING_T", "BYTEARRAY_T", "INTARRAY_T", 
      "BYTE_T", "UNSIGNED_T", "INTEGER_T", "FLOAT_T", "SELECT", "STREAM", 
      "FROM", "DECLARE", "FILE", "MIN", "MAX", "AVG", "SUMC", "ID", "STRING", 
      "FLOAT", "DECIMAL", "REAL", "EQUAL", "GREATER", "LESS", "EXCLAMATION", 
      "DOUBLE_BAR", "DOT", "UNDERLINE", "AT", "SHARP", "AND", "MOD", "DOLLAR", 
      "COMMA", "SEMI", "COLON", "DOUBLE_COLON", "STAR", "DIVIDE", "PLUS", 
      "MINUS", "BIT_NOT", "BIT_OR", "BIT_XOR", "SPACE", "COMMENT", "LINE_COMMENT", 
      "LETTER", "DEC_DOT_DEC", "HEX_DIGIT", "DEC_DIGIT"
    },
    std::vector<std::string>{
      "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
    },
    std::vector<std::string>{
      "DEFAULT_MODE"
    },
    std::vector<std::string>{
      "", "'['", "']'", "'('", "')'", "'Sqrt'", "'Ceil'", "'Abs'", "'Floor'", 
      "'Sign'", "'Chr'", "'Length'", "'ToNumber'", "'ToTimeStamp'", "'FloatCast'", 
      "'InstCast'", "'Count'", "'Crc'", "'Sum'", "'IsZero'", "'IsNonZero'", 
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "'='", "'>'", "'<'", "'!'", "'||'", "'.'", "'_'", 
      "'@'", "'#'", "'&'", "'%'", "'$'", "','", "';'", "':'", "'::'", "'*'", 
      "'/'", "'+'", "'-'", "'~'", "'|'", "'^'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "STRING_T", "BYTEARRAY_T", "INTARRAY_T", "BYTE_T", 
      "UNSIGNED_T", "INTEGER_T", "FLOAT_T", "SELECT", "STREAM", "FROM", 
      "DECLARE", "FILE", "MIN", "MAX", "AVG", "SUMC", "ID", "STRING", "FLOAT", 
      "DECIMAL", "REAL", "EQUAL", "GREATER", "LESS", "EXCLAMATION", "DOUBLE_BAR", 
      "DOT", "UNDERLINE", "AT", "SHARP", "AND", "MOD", "DOLLAR", "COMMA", 
      "SEMI", "COLON", "DOUBLE_COLON", "STAR", "DIVIDE", "PLUS", "MINUS", 
      "BIT_NOT", "BIT_OR", "BIT_XOR", "SPACE", "COMMENT", "LINE_COMMENT"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,0,67,610,6,-1,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,
  	6,2,7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,
  	7,14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,
  	7,21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,
  	7,28,2,29,7,29,2,30,7,30,2,31,7,31,2,32,7,32,2,33,7,33,2,34,7,34,2,35,
  	7,35,2,36,7,36,2,37,7,37,2,38,7,38,2,39,7,39,2,40,7,40,2,41,7,41,2,42,
  	7,42,2,43,7,43,2,44,7,44,2,45,7,45,2,46,7,46,2,47,7,47,2,48,7,48,2,49,
  	7,49,2,50,7,50,2,51,7,51,2,52,7,52,2,53,7,53,2,54,7,54,2,55,7,55,2,56,
  	7,56,2,57,7,57,2,58,7,58,2,59,7,59,2,60,7,60,2,61,7,61,2,62,7,62,2,63,
  	7,63,2,64,7,64,2,65,7,65,2,66,7,66,2,67,7,67,2,68,7,68,2,69,7,69,2,70,
  	7,70,1,0,1,0,1,1,1,1,1,2,1,2,1,3,1,3,1,4,1,4,1,4,1,4,1,4,1,5,1,5,1,5,
  	1,5,1,5,1,6,1,6,1,6,1,6,1,7,1,7,1,7,1,7,1,7,1,7,1,8,1,8,1,8,1,8,1,8,1,
  	9,1,9,1,9,1,9,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,11,1,11,1,11,1,11,
  	1,11,1,11,1,11,1,11,1,11,1,12,1,12,1,12,1,12,1,12,1,12,1,12,1,12,1,12,
  	1,12,1,12,1,12,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,14,
  	1,14,1,14,1,14,1,14,1,14,1,14,1,14,1,14,1,15,1,15,1,15,1,15,1,15,1,15,
  	1,16,1,16,1,16,1,16,1,17,1,17,1,17,1,17,1,18,1,18,1,18,1,18,1,18,1,18,
  	1,18,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,20,1,20,1,20,
  	1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,3,20,271,8,20,1,21,1,21,
  	1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,
  	1,21,1,21,3,21,291,8,21,1,22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,
  	1,22,1,22,1,22,1,22,1,22,1,22,1,22,3,22,309,8,22,1,23,1,23,1,23,1,23,
  	1,23,1,23,1,23,1,23,3,23,319,8,23,1,24,1,24,1,24,1,24,1,24,1,24,1,24,
  	1,24,1,24,1,24,1,24,1,24,3,24,333,8,24,1,25,1,25,1,25,1,25,1,25,1,25,
  	1,25,1,25,1,25,1,25,1,25,1,25,1,25,3,25,348,8,25,1,26,1,26,1,26,1,26,
  	1,26,1,26,1,26,1,26,1,26,1,26,3,26,360,8,26,1,27,1,27,1,27,1,27,1,27,
  	1,27,1,27,1,27,1,27,1,27,1,27,1,27,3,27,374,8,27,1,28,1,28,1,28,1,28,
  	1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,3,28,388,8,28,1,29,1,29,1,29,
  	1,29,1,29,1,29,1,29,1,29,3,29,398,8,29,1,30,1,30,1,30,1,30,1,30,1,30,
  	1,30,1,30,1,30,1,30,1,30,1,30,1,30,1,30,3,30,414,8,30,1,31,1,31,1,31,
  	1,31,1,31,1,31,1,31,1,31,3,31,424,8,31,1,32,1,32,1,32,1,32,1,32,1,32,
  	3,32,432,8,32,1,33,1,33,1,33,1,33,1,33,1,33,3,33,440,8,33,1,34,1,34,1,
  	34,1,34,1,34,1,34,3,34,448,8,34,1,35,1,35,1,35,1,35,1,35,1,35,1,35,1,
  	35,3,35,458,8,35,1,36,1,36,5,36,462,8,36,10,36,12,36,465,9,36,1,37,1,
  	37,1,37,1,37,5,37,471,8,37,10,37,12,37,474,9,37,1,37,1,37,1,38,1,38,1,
  	39,4,39,481,8,39,11,39,12,39,482,1,40,1,40,3,40,487,8,40,1,40,1,40,3,
  	40,491,8,40,1,40,4,40,494,8,40,11,40,12,40,495,1,41,1,41,1,42,1,42,1,
  	43,1,43,1,44,1,44,1,45,1,45,1,45,1,46,1,46,1,47,1,47,1,48,1,48,1,49,1,
  	49,1,50,1,50,1,51,1,51,1,52,1,52,1,53,1,53,1,54,1,54,1,55,1,55,1,56,1,
  	56,1,56,1,57,1,57,1,58,1,58,1,59,1,59,1,60,1,60,1,61,1,61,1,62,1,62,1,
  	63,1,63,1,64,4,64,547,8,64,11,64,12,64,548,1,64,1,64,1,65,1,65,1,65,1,
  	65,1,65,5,65,558,8,65,10,65,12,65,561,9,65,1,65,1,65,1,65,1,65,1,65,1,
  	66,1,66,1,66,1,66,5,66,572,8,66,10,66,12,66,575,9,66,1,66,1,66,1,67,1,
  	67,1,68,4,68,582,8,68,11,68,12,68,583,1,68,1,68,4,68,588,8,68,11,68,12,
  	68,589,1,68,4,68,593,8,68,11,68,12,68,594,1,68,1,68,1,68,1,68,4,68,601,
  	8,68,11,68,12,68,602,3,68,605,8,68,1,69,1,69,1,70,1,70,1,559,0,71,1,1,
  	3,2,5,3,7,4,9,5,11,6,13,7,15,8,17,9,19,10,21,11,23,12,25,13,27,14,29,
  	15,31,16,33,17,35,18,37,19,39,20,41,21,43,22,45,23,47,24,49,25,51,26,
  	53,27,55,28,57,29,59,30,61,31,63,32,65,33,67,34,69,35,71,36,73,37,75,
  	38,77,39,79,40,81,41,83,42,85,43,87,44,89,45,91,46,93,47,95,48,97,49,
  	99,50,101,51,103,52,105,53,107,54,109,55,111,56,113,57,115,58,117,59,
  	119,60,121,61,123,62,125,63,127,64,129,65,131,66,133,67,135,0,137,0,139,
  	0,141,0,1,0,9,2,0,65,90,97,122,5,0,36,36,48,57,65,90,95,95,97,122,1,0,
  	39,39,2,0,43,43,45,45,3,0,9,10,13,13,32,32,2,0,10,10,13,13,2,0,65,90,
  	95,95,2,0,48,57,65,70,1,0,48,57,639,0,1,1,0,0,0,0,3,1,0,0,0,0,5,1,0,0,
  	0,0,7,1,0,0,0,0,9,1,0,0,0,0,11,1,0,0,0,0,13,1,0,0,0,0,15,1,0,0,0,0,17,
  	1,0,0,0,0,19,1,0,0,0,0,21,1,0,0,0,0,23,1,0,0,0,0,25,1,0,0,0,0,27,1,0,
  	0,0,0,29,1,0,0,0,0,31,1,0,0,0,0,33,1,0,0,0,0,35,1,0,0,0,0,37,1,0,0,0,
  	0,39,1,0,0,0,0,41,1,0,0,0,0,43,1,0,0,0,0,45,1,0,0,0,0,47,1,0,0,0,0,49,
  	1,0,0,0,0,51,1,0,0,0,0,53,1,0,0,0,0,55,1,0,0,0,0,57,1,0,0,0,0,59,1,0,
  	0,0,0,61,1,0,0,0,0,63,1,0,0,0,0,65,1,0,0,0,0,67,1,0,0,0,0,69,1,0,0,0,
  	0,71,1,0,0,0,0,73,1,0,0,0,0,75,1,0,0,0,0,77,1,0,0,0,0,79,1,0,0,0,0,81,
  	1,0,0,0,0,83,1,0,0,0,0,85,1,0,0,0,0,87,1,0,0,0,0,89,1,0,0,0,0,91,1,0,
  	0,0,0,93,1,0,0,0,0,95,1,0,0,0,0,97,1,0,0,0,0,99,1,0,0,0,0,101,1,0,0,0,
  	0,103,1,0,0,0,0,105,1,0,0,0,0,107,1,0,0,0,0,109,1,0,0,0,0,111,1,0,0,0,
  	0,113,1,0,0,0,0,115,1,0,0,0,0,117,1,0,0,0,0,119,1,0,0,0,0,121,1,0,0,0,
  	0,123,1,0,0,0,0,125,1,0,0,0,0,127,1,0,0,0,0,129,1,0,0,0,0,131,1,0,0,0,
  	0,133,1,0,0,0,1,143,1,0,0,0,3,145,1,0,0,0,5,147,1,0,0,0,7,149,1,0,0,0,
  	9,151,1,0,0,0,11,156,1,0,0,0,13,161,1,0,0,0,15,165,1,0,0,0,17,171,1,0,
  	0,0,19,176,1,0,0,0,21,180,1,0,0,0,23,187,1,0,0,0,25,196,1,0,0,0,27,208,
  	1,0,0,0,29,218,1,0,0,0,31,227,1,0,0,0,33,233,1,0,0,0,35,237,1,0,0,0,37,
  	241,1,0,0,0,39,248,1,0,0,0,41,270,1,0,0,0,43,290,1,0,0,0,45,308,1,0,0,
  	0,47,318,1,0,0,0,49,332,1,0,0,0,51,347,1,0,0,0,53,359,1,0,0,0,55,373,
  	1,0,0,0,57,387,1,0,0,0,59,397,1,0,0,0,61,413,1,0,0,0,63,423,1,0,0,0,65,
  	431,1,0,0,0,67,439,1,0,0,0,69,447,1,0,0,0,71,457,1,0,0,0,73,459,1,0,0,
  	0,75,466,1,0,0,0,77,477,1,0,0,0,79,480,1,0,0,0,81,486,1,0,0,0,83,497,
  	1,0,0,0,85,499,1,0,0,0,87,501,1,0,0,0,89,503,1,0,0,0,91,505,1,0,0,0,93,
  	508,1,0,0,0,95,510,1,0,0,0,97,512,1,0,0,0,99,514,1,0,0,0,101,516,1,0,
  	0,0,103,518,1,0,0,0,105,520,1,0,0,0,107,522,1,0,0,0,109,524,1,0,0,0,111,
  	526,1,0,0,0,113,528,1,0,0,0,115,531,1,0,0,0,117,533,1,0,0,0,119,535,1,
  	0,0,0,121,537,1,0,0,0,123,539,1,0,0,0,125,541,1,0,0,0,127,543,1,0,0,0,
  	129,546,1,0,0,0,131,552,1,0,0,0,133,567,1,0,0,0,135,578,1,0,0,0,137,604,
  	1,0,0,0,139,606,1,0,0,0,141,608,1,0,0,0,143,144,5,91,0,0,144,2,1,0,0,
  	0,145,146,5,93,0,0,146,4,1,0,0,0,147,148,5,40,0,0,148,6,1,0,0,0,149,150,
  	5,41,0,0,150,8,1,0,0,0,151,152,5,83,0,0,152,153,5,113,0,0,153,154,5,114,
  	0,0,154,155,5,116,0,0,155,10,1,0,0,0,156,157,5,67,0,0,157,158,5,101,0,
  	0,158,159,5,105,0,0,159,160,5,108,0,0,160,12,1,0,0,0,161,162,5,65,0,0,
  	162,163,5,98,0,0,163,164,5,115,0,0,164,14,1,0,0,0,165,166,5,70,0,0,166,
  	167,5,108,0,0,167,168,5,111,0,0,168,169,5,111,0,0,169,170,5,114,0,0,170,
  	16,1,0,0,0,171,172,5,83,0,0,172,173,5,105,0,0,173,174,5,103,0,0,174,175,
  	5,110,0,0,175,18,1,0,0,0,176,177,5,67,0,0,177,178,5,104,0,0,178,179,5,
  	114,0,0,179,20,1,0,0,0,180,181,5,76,0,0,181,182,5,101,0,0,182,183,5,110,
  	0,0,183,184,5,103,0,0,184,185,5,116,0,0,185,186,5,104,0,0,186,22,1,0,
  	0,0,187,188,5,84,0,0,188,189,5,111,0,0,189,190,5,78,0,0,190,191,5,117,
  	0,0,191,192,5,109,0,0,192,193,5,98,0,0,193,194,5,101,0,0,194,195,5,114,
  	0,0,195,24,1,0,0,0,196,197,5,84,0,0,197,198,5,111,0,0,198,199,5,84,0,
  	0,199,200,5,105,0,0,200,201,5,109,0,0,201,202,5,101,0,0,202,203,5,83,
  	0,0,203,204,5,116,0,0,204,205,5,97,0,0,205,206,5,109,0,0,206,207,5,112,
  	0,0,207,26,1,0,0,0,208,209,5,70,0,0,209,210,5,108,0,0,210,211,5,111,0,
  	0,211,212,5,97,0,0,212,213,5,116,0,0,213,214,5,67,0,0,214,215,5,97,0,
  	0,215,216,5,115,0,0,216,217,5,116,0,0,217,28,1,0,0,0,218,219,5,73,0,0,
  	219,220,5,110,0,0,220,221,5,115,0,0,221,222,5,116,0,0,222,223,5,67,0,
  	0,223,224,5,97,0,0,224,225,5,115,0,0,225,226,5,116,0,0,226,30,1,0,0,0,
  	227,228,5,67,0,0,228,229,5,111,0,0,229,230,5,117,0,0,230,231,5,110,0,
  	0,231,232,5,116,0,0,232,32,1,0,0,0,233,234,5,67,0,0,234,235,5,114,0,0,
  	235,236,5,99,0,0,236,34,1,0,0,0,237,238,5,83,0,0,238,239,5,117,0,0,239,
  	240,5,109,0,0,240,36,1,0,0,0,241,242,5,73,0,0,242,243,5,115,0,0,243,244,
  	5,90,0,0,244,245,5,101,0,0,245,246,5,114,0,0,246,247,5,111,0,0,247,38,
  	1,0,0,0,248,249,5,73,0,0,249,250,5,115,0,0,250,251,5,78,0,0,251,252,5,
  	111,0,0,252,253,5,110,0,0,253,254,5,90,0,0,254,255,5,101,0,0,255,256,
  	5,114,0,0,256,257,5,111,0,0,257,40,1,0,0,0,258,259,5,83,0,0,259,260,5,
  	84,0,0,260,261,5,82,0,0,261,262,5,73,0,0,262,263,5,78,0,0,263,271,5,71,
  	0,0,264,265,5,83,0,0,265,266,5,116,0,0,266,267,5,114,0,0,267,268,5,105,
  	0,0,268,269,5,110,0,0,269,271,5,103,0,0,270,258,1,0,0,0,270,264,1,0,0,
  	0,271,42,1,0,0,0,272,273,5,66,0,0,273,274,5,89,0,0,274,275,5,84,0,0,275,
  	276,5,69,0,0,276,277,5,65,0,0,277,278,5,82,0,0,278,279,5,82,0,0,279,280,
  	5,65,0,0,280,291,5,89,0,0,281,282,5,66,0,0,282,283,5,121,0,0,283,284,
  	5,116,0,0,284,285,5,101,0,0,285,286,5,97,0,0,286,287,5,114,0,0,287,288,
  	5,114,0,0,288,289,5,97,0,0,289,291,5,121,0,0,290,272,1,0,0,0,290,281,
  	1,0,0,0,291,44,1,0,0,0,292,293,5,73,0,0,293,294,5,78,0,0,294,295,5,84,
  	0,0,295,296,5,65,0,0,296,297,5,82,0,0,297,298,5,82,0,0,298,299,5,65,0,
  	0,299,309,5,89,0,0,300,301,5,73,0,0,301,302,5,110,0,0,302,303,5,116,0,
  	0,303,304,5,97,0,0,304,305,5,114,0,0,305,306,5,114,0,0,306,307,5,97,0,
  	0,307,309,5,121,0,0,308,292,1,0,0,0,308,300,1,0,0,0,309,46,1,0,0,0,310,
  	311,5,66,0,0,311,312,5,89,0,0,312,313,5,84,0,0,313,319,5,69,0,0,314,315,
  	5,66,0,0,315,316,5,121,0,0,316,317,5,116,0,0,317,319,5,101,0,0,318,310,
  	1,0,0,0,318,314,1,0,0,0,319,48,1,0,0,0,320,321,5,85,0,0,321,322,5,78,
  	0,0,322,323,5,83,0,0,323,324,5,73,0,0,324,325,5,71,0,0,325,326,5,78,0,
  	0,326,327,5,69,0,0,327,333,5,68,0,0,328,329,5,85,0,0,329,330,5,105,0,
  	0,330,331,5,110,0,0,331,333,5,116,0,0,332,320,1,0,0,0,332,328,1,0,0,0,
  	333,50,1,0,0,0,334,335,5,73,0,0,335,336,5,78,0,0,336,348,5,84,0,0,337,
  	338,5,73,0,0,338,339,5,110,0,0,339,348,5,116,0,0,340,341,5,73,0,0,341,
  	342,5,78,0,0,342,343,5,84,0,0,343,344,5,69,0,0,344,345,5,71,0,0,345,346,
  	5,69,0,0,346,348,5,82,0,0,347,334,1,0,0,0,347,337,1,0,0,0,347,340,1,0,
  	0,0,348,52,1,0,0,0,349,350,5,70,0,0,350,351,5,76,0,0,351,352,5,79,0,0,
  	352,353,5,65,0,0,353,360,5,84,0,0,354,355,5,70,0,0,355,356,5,108,0,0,
  	356,357,5,111,0,0,357,358,5,97,0,0,358,360,5,116,0,0,359,349,1,0,0,0,
  	359,354,1,0,0,0,360,54,1,0,0,0,361,362,5,83,0,0,362,363,5,69,0,0,363,
  	364,5,76,0,0,364,365,5,69,0,0,365,366,5,67,0,0,366,374,5,84,0,0,367,368,
  	5,115,0,0,368,369,5,101,0,0,369,370,5,108,0,0,370,371,5,101,0,0,371,372,
  	5,99,0,0,372,374,5,116,0,0,373,361,1,0,0,0,373,367,1,0,0,0,374,56,1,0,
  	0,0,375,376,5,83,0,0,376,377,5,84,0,0,377,378,5,82,0,0,378,379,5,69,0,
  	0,379,380,5,65,0,0,380,388,5,77,0,0,381,382,5,115,0,0,382,383,5,116,0,
  	0,383,384,5,114,0,0,384,385,5,101,0,0,385,386,5,97,0,0,386,388,5,109,
  	0,0,387,375,1,0,0,0,387,381,1,0,0,0,388,58,1,0,0,0,389,390,5,70,0,0,390,
  	391,5,82,0,0,391,392,5,79,0,0,392,398,5,77,0,0,393,394,5,102,0,0,394,
  	395,5,114,0,0,395,396,5,111,0,0,396,398,5,109,0,0,397,389,1,0,0,0,397,
  	393,1,0,0,0,398,60,1,0,0,0,399,400,5,68,0,0,400,401,5,69,0,0,401,402,
  	5,67,0,0,402,403,5,76,0,0,403,404,5,65,0,0,404,405,5,82,0,0,405,414,5,
  	69,0,0,406,407,5,100,0,0,407,408,5,101,0,0,408,409,5,99,0,0,409,410,5,
  	108,0,0,410,411,5,97,0,0,411,412,5,114,0,0,412,414,5,101,0,0,413,399,
  	1,0,0,0,413,406,1,0,0,0,414,62,1,0,0,0,415,416,5,70,0,0,416,417,5,73,
  	0,0,417,418,5,76,0,0,418,424,5,69,0,0,419,420,5,102,0,0,420,421,5,105,
  	0,0,421,422,5,108,0,0,422,424,5,101,0,0,423,415,1,0,0,0,423,419,1,0,0,
  	0,424,64,1,0,0,0,425,426,5,77,0,0,426,427,5,73,0,0,427,432,5,78,0,0,428,
  	429,5,109,0,0,429,430,5,105,0,0,430,432,5,110,0,0,431,425,1,0,0,0,431,
  	428,1,0,0,0,432,66,1,0,0,0,433,434,5,77,0,0,434,435,5,65,0,0,435,440,
  	5,88,0,0,436,437,5,109,0,0,437,438,5,97,0,0,438,440,5,120,0,0,439,433,
  	1,0,0,0,439,436,1,0,0,0,440,68,1,0,0,0,441,442,5,65,0,0,442,443,5,86,
  	0,0,443,448,5,71,0,0,444,445,5,97,0,0,445,446,5,118,0,0,446,448,5,103,
  	0,0,447,441,1,0,0,0,447,444,1,0,0,0,448,70,1,0,0,0,449,450,5,83,0,0,450,
  	451,5,85,0,0,451,452,5,77,0,0,452,458,5,67,0,0,453,454,5,115,0,0,454,
  	455,5,117,0,0,455,456,5,109,0,0,456,458,5,99,0,0,457,449,1,0,0,0,457,
  	453,1,0,0,0,458,72,1,0,0,0,459,463,7,0,0,0,460,462,7,1,0,0,461,460,1,
  	0,0,0,462,465,1,0,0,0,463,461,1,0,0,0,463,464,1,0,0,0,464,74,1,0,0,0,
  	465,463,1,0,0,0,466,472,5,39,0,0,467,471,8,2,0,0,468,469,5,39,0,0,469,
  	471,5,39,0,0,470,467,1,0,0,0,470,468,1,0,0,0,471,474,1,0,0,0,472,470,
  	1,0,0,0,472,473,1,0,0,0,473,475,1,0,0,0,474,472,1,0,0,0,475,476,5,39,
  	0,0,476,76,1,0,0,0,477,478,3,137,68,0,478,78,1,0,0,0,479,481,3,141,70,
  	0,480,479,1,0,0,0,481,482,1,0,0,0,482,480,1,0,0,0,482,483,1,0,0,0,483,
  	80,1,0,0,0,484,487,3,79,39,0,485,487,3,137,68,0,486,484,1,0,0,0,486,485,
  	1,0,0,0,487,488,1,0,0,0,488,490,5,69,0,0,489,491,7,3,0,0,490,489,1,0,
  	0,0,490,491,1,0,0,0,491,493,1,0,0,0,492,494,3,141,70,0,493,492,1,0,0,
  	0,494,495,1,0,0,0,495,493,1,0,0,0,495,496,1,0,0,0,496,82,1,0,0,0,497,
  	498,5,61,0,0,498,84,1,0,0,0,499,500,5,62,0,0,500,86,1,0,0,0,501,502,5,
  	60,0,0,502,88,1,0,0,0,503,504,5,33,0,0,504,90,1,0,0,0,505,506,5,124,0,
  	0,506,507,5,124,0,0,507,92,1,0,0,0,508,509,5,46,0,0,509,94,1,0,0,0,510,
  	511,5,95,0,0,511,96,1,0,0,0,512,513,5,64,0,0,513,98,1,0,0,0,514,515,5,
  	35,0,0,515,100,1,0,0,0,516,517,5,38,0,0,517,102,1,0,0,0,518,519,5,37,
  	0,0,519,104,1,0,0,0,520,521,5,36,0,0,521,106,1,0,0,0,522,523,5,44,0,0,
  	523,108,1,0,0,0,524,525,5,59,0,0,525,110,1,0,0,0,526,527,5,58,0,0,527,
  	112,1,0,0,0,528,529,5,58,0,0,529,530,5,58,0,0,530,114,1,0,0,0,531,532,
  	5,42,0,0,532,116,1,0,0,0,533,534,5,47,0,0,534,118,1,0,0,0,535,536,5,43,
  	0,0,536,120,1,0,0,0,537,538,5,45,0,0,538,122,1,0,0,0,539,540,5,126,0,
  	0,540,124,1,0,0,0,541,542,5,124,0,0,542,126,1,0,0,0,543,544,5,94,0,0,
  	544,128,1,0,0,0,545,547,7,4,0,0,546,545,1,0,0,0,547,548,1,0,0,0,548,546,
  	1,0,0,0,548,549,1,0,0,0,549,550,1,0,0,0,550,551,6,64,0,0,551,130,1,0,
  	0,0,552,553,5,47,0,0,553,554,5,42,0,0,554,559,1,0,0,0,555,558,3,131,65,
  	0,556,558,9,0,0,0,557,555,1,0,0,0,557,556,1,0,0,0,558,561,1,0,0,0,559,
  	560,1,0,0,0,559,557,1,0,0,0,560,562,1,0,0,0,561,559,1,0,0,0,562,563,5,
  	42,0,0,563,564,5,47,0,0,564,565,1,0,0,0,565,566,6,65,1,0,566,132,1,0,
  	0,0,567,568,5,35,0,0,568,569,5,32,0,0,569,573,1,0,0,0,570,572,8,5,0,0,
  	571,570,1,0,0,0,572,575,1,0,0,0,573,571,1,0,0,0,573,574,1,0,0,0,574,576,
  	1,0,0,0,575,573,1,0,0,0,576,577,6,66,1,0,577,134,1,0,0,0,578,579,7,6,
  	0,0,579,136,1,0,0,0,580,582,3,141,70,0,581,580,1,0,0,0,582,583,1,0,0,
  	0,583,581,1,0,0,0,583,584,1,0,0,0,584,585,1,0,0,0,585,587,5,46,0,0,586,
  	588,3,141,70,0,587,586,1,0,0,0,588,589,1,0,0,0,589,587,1,0,0,0,589,590,
  	1,0,0,0,590,605,1,0,0,0,591,593,3,141,70,0,592,591,1,0,0,0,593,594,1,
  	0,0,0,594,592,1,0,0,0,594,595,1,0,0,0,595,596,1,0,0,0,596,597,5,46,0,
  	0,597,605,1,0,0,0,598,600,5,46,0,0,599,601,3,141,70,0,600,599,1,0,0,0,
  	601,602,1,0,0,0,602,600,1,0,0,0,602,603,1,0,0,0,603,605,1,0,0,0,604,581,
  	1,0,0,0,604,592,1,0,0,0,604,598,1,0,0,0,605,138,1,0,0,0,606,607,7,7,0,
  	0,607,140,1,0,0,0,608,609,7,8,0,0,609,142,1,0,0,0,33,0,270,290,308,318,
  	332,347,359,373,387,397,413,423,431,439,447,457,463,470,472,482,486,490,
  	495,548,557,559,573,583,589,594,602,604,2,6,0,0,0,1,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  rqllexerLexerStaticData = staticData.release();
}

}

RQLLexer::RQLLexer(CharStream *input) : Lexer(input) {
  RQLLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *rqllexerLexerStaticData->atn, rqllexerLexerStaticData->decisionToDFA, rqllexerLexerStaticData->sharedContextCache);
}

RQLLexer::~RQLLexer() {
  delete _interpreter;
}

std::string RQLLexer::getGrammarFileName() const {
  return "RQL.g4";
}

const std::vector<std::string>& RQLLexer::getRuleNames() const {
  return rqllexerLexerStaticData->ruleNames;
}

const std::vector<std::string>& RQLLexer::getChannelNames() const {
  return rqllexerLexerStaticData->channelNames;
}

const std::vector<std::string>& RQLLexer::getModeNames() const {
  return rqllexerLexerStaticData->modeNames;
}

const dfa::Vocabulary& RQLLexer::getVocabulary() const {
  return rqllexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView RQLLexer::getSerializedATN() const {
  return rqllexerLexerStaticData->serializedATN;
}

const atn::ATN& RQLLexer::getATN() const {
  return *rqllexerLexerStaticData->atn;
}




void RQLLexer::initialize() {
  std::call_once(rqllexerLexerOnceFlag, rqllexerLexerInitialize);
}
