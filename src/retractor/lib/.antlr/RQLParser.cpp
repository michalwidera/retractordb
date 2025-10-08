
// Generated from RQL.g4 by ANTLR 4.13.1


#include "RQLListener.h"

#include "RQLParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct RQLParserStaticData final {
  RQLParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  RQLParserStaticData(const RQLParserStaticData&) = delete;
  RQLParserStaticData(RQLParserStaticData&&) = delete;
  RQLParserStaticData& operator=(const RQLParserStaticData&) = delete;
  RQLParserStaticData& operator=(RQLParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag rqlParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
RQLParserStaticData *rqlParserStaticData = nullptr;

void rqlParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (rqlParserStaticData != nullptr) {
    return;
  }
#else
  assert(rqlParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<RQLParserStaticData>(
    std::vector<std::string>{
      "prog", "compiler_option", "storage_statement", "substrat_statement", 
      "select_statement", "declare_statement", "rule_statement", "dumppart", 
      "systempart", "rational_se", "retention_from", "fraction_rule", "field_declaration", 
      "field_type", "select_list", "field_id", "unary_op_expression", "asterisk", 
      "expression", "logic", "expression_logic", "term_logic", "expression_factor", 
      "term", "stream_expression", "stream_term", "stream_factor", "agregator", 
      "function_call"
    },
    std::vector<std::string>{
      "", "'['", "']'", "'('", "')'", "'Sqrt'", "'Ceil'", "'Abs'", "'Floor'", 
      "'Sign'", "'Chr'", "'Length'", "'ToNumber'", "'ToTimeStamp'", "'FloatCast'", 
      "'IntCast'", "'Count'", "'Crc'", "'Sum'", "'IsZero'", "'IsNonZero'", 
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "'='", "'!='", "'>'", "'<'", "'>='", "'<='", "'!'", "'||'", "'.'", 
      "'_'", "'@'", "'#'", "'&'", "'%'", "'$'", "','", "';'", "':'", "'::'", 
      "'*'", "'/'", "'+'", "'-'", "'~'", "'|'", "'^'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "BYTE_T", "STRING_T", "UNSIGNED_T", "INTEGER_T", "FLOAT_T", 
      "DOUBLE_T", "SELECT", "STREAM", "FROM", "DECLARE", "RETENTION", "FILE", 
      "STORAGE", "COPTION", "SUBSTRAT", "RULE", "ON", "WHEN", "DUMP", "SYSTEM", 
      "DO", "TO", "AND_C", "OR_C", "NOT_C", "MIN", "MAX", "AVG", "SUMC", 
      "STRING_SUBSTRAT", "ID", "STRING", "FLOAT", "DECIMAL", "REAL", "IS_EQ", 
      "IS_NQ", "IS_GR", "IS_LS", "IS_GE", "IS_LE", "EXCLAMATION", "DOUBLE_BAR", 
      "DOT", "UNDERLINE", "AT", "SHARP", "AND", "MOD", "DOLLAR", "COMMA", 
      "SEMI", "COLON", "DOUBLE_COLON", "STAR", "DIVIDE", "PLUS", "MINUS", 
      "BIT_NOT", "BIT_OR", "BIT_XOR", "SPACE", "COMMENT", "LINE_COMMENT1", 
      "LINE_COMMENT2"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,85,365,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,7,
  	28,1,0,1,0,1,0,1,0,1,0,1,0,4,0,65,8,0,11,0,12,0,66,1,0,1,0,1,1,1,1,1,
  	1,1,2,1,2,1,2,1,3,1,3,1,3,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,88,8,4,
  	1,4,3,4,91,8,4,1,5,1,5,1,5,1,5,5,5,97,8,5,10,5,12,5,100,9,5,1,5,1,5,1,
  	5,1,5,1,5,1,5,1,5,1,6,1,6,1,6,1,6,1,6,1,6,1,6,1,6,1,6,3,6,118,8,6,1,7,
  	1,7,3,7,122,8,7,1,7,1,7,1,7,3,7,127,8,7,1,7,1,7,1,7,3,7,132,8,7,1,8,1,
  	8,1,8,1,9,1,9,1,9,3,9,140,8,9,1,10,1,10,1,10,3,10,145,8,10,1,11,1,11,
  	1,11,1,11,1,12,1,12,1,12,1,12,1,12,3,12,156,8,12,1,13,1,13,1,13,1,13,
  	1,13,1,13,3,13,164,8,13,1,14,1,14,1,14,1,14,5,14,170,8,14,10,14,12,14,
  	173,9,14,3,14,175,8,14,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,
  	15,1,15,1,15,3,15,189,8,15,1,16,1,16,1,16,1,16,3,16,195,8,16,1,17,1,17,
  	3,17,199,8,17,1,17,1,17,1,18,1,18,1,19,1,19,1,20,1,20,1,20,1,20,1,20,
  	1,20,1,20,1,20,1,20,5,20,216,8,20,10,20,12,20,219,9,20,1,21,1,21,1,21,
  	1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,
  	1,21,1,21,1,21,1,21,5,21,242,8,21,10,21,12,21,245,9,21,1,22,1,22,1,22,
  	1,22,1,22,1,22,1,22,1,22,1,22,5,22,256,8,22,10,22,12,22,259,9,22,1,23,
  	1,23,1,23,1,23,1,23,1,23,1,23,3,23,268,8,23,1,23,1,23,3,23,272,8,23,1,
  	23,1,23,1,23,1,23,1,23,1,23,1,23,1,23,3,23,282,8,23,1,23,1,23,1,23,1,
  	23,1,23,1,23,5,23,290,8,23,10,23,12,23,293,9,23,1,24,1,24,1,24,1,24,1,
  	24,1,24,1,24,1,24,1,24,1,24,1,24,1,24,1,24,3,24,308,8,24,1,25,1,25,1,
  	25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,
  	25,1,25,3,25,328,8,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,3,25,338,
  	8,25,1,26,1,26,1,26,1,26,1,26,3,26,345,8,26,1,27,1,27,1,27,1,27,3,27,
  	351,8,27,1,28,1,28,1,28,1,28,1,28,5,28,358,8,28,10,28,12,28,361,9,28,
  	1,28,1,28,1,28,0,4,40,42,44,46,29,0,2,4,6,8,10,12,14,16,18,20,22,24,26,
  	28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,0,2,1,0,77,78,1,0,5,20,401,
  	0,64,1,0,0,0,2,70,1,0,0,0,4,73,1,0,0,0,6,76,1,0,0,0,8,79,1,0,0,0,10,92,
  	1,0,0,0,12,108,1,0,0,0,14,119,1,0,0,0,16,133,1,0,0,0,18,139,1,0,0,0,20,
  	141,1,0,0,0,22,146,1,0,0,0,24,150,1,0,0,0,26,163,1,0,0,0,28,174,1,0,0,
  	0,30,188,1,0,0,0,32,194,1,0,0,0,34,198,1,0,0,0,36,202,1,0,0,0,38,204,
  	1,0,0,0,40,206,1,0,0,0,42,220,1,0,0,0,44,246,1,0,0,0,46,281,1,0,0,0,48,
  	307,1,0,0,0,50,337,1,0,0,0,52,344,1,0,0,0,54,350,1,0,0,0,56,352,1,0,0,
  	0,58,65,3,8,4,0,59,65,3,10,5,0,60,65,3,4,2,0,61,65,3,6,3,0,62,65,3,2,
  	1,0,63,65,3,12,6,0,64,58,1,0,0,0,64,59,1,0,0,0,64,60,1,0,0,0,64,61,1,
  	0,0,0,64,62,1,0,0,0,64,63,1,0,0,0,65,66,1,0,0,0,66,64,1,0,0,0,66,67,1,
  	0,0,0,67,68,1,0,0,0,68,69,5,0,0,1,69,1,1,0,0,0,70,71,5,34,0,0,71,72,5,
  	52,0,0,72,3,1,0,0,0,73,74,5,33,0,0,74,75,5,52,0,0,75,5,1,0,0,0,76,77,
  	5,35,0,0,77,78,5,50,0,0,78,7,1,0,0,0,79,80,5,27,0,0,80,81,3,28,14,0,81,
  	82,5,28,0,0,82,83,5,51,0,0,83,84,5,29,0,0,84,87,3,48,24,0,85,86,5,32,
  	0,0,86,88,5,52,0,0,87,85,1,0,0,0,87,88,1,0,0,0,88,90,1,0,0,0,89,91,3,
  	20,10,0,90,89,1,0,0,0,90,91,1,0,0,0,91,9,1,0,0,0,92,93,5,30,0,0,93,98,
  	3,24,12,0,94,95,5,71,0,0,95,97,3,24,12,0,96,94,1,0,0,0,97,100,1,0,0,0,
  	98,96,1,0,0,0,98,99,1,0,0,0,99,101,1,0,0,0,100,98,1,0,0,0,101,102,5,28,
  	0,0,102,103,5,51,0,0,103,104,5,71,0,0,104,105,3,18,9,0,105,106,5,32,0,
  	0,106,107,5,52,0,0,107,11,1,0,0,0,108,109,5,36,0,0,109,110,5,51,0,0,110,
  	111,5,37,0,0,111,112,5,51,0,0,112,113,5,38,0,0,113,114,3,38,19,0,114,
  	117,5,41,0,0,115,118,3,14,7,0,116,118,3,16,8,0,117,115,1,0,0,0,117,116,
  	1,0,0,0,118,13,1,0,0,0,119,121,5,39,0,0,120,122,5,78,0,0,121,120,1,0,
  	0,0,121,122,1,0,0,0,122,123,1,0,0,0,123,124,5,54,0,0,124,126,5,42,0,0,
  	125,127,5,78,0,0,126,125,1,0,0,0,126,127,1,0,0,0,127,128,1,0,0,0,128,
  	131,5,54,0,0,129,130,5,31,0,0,130,132,5,54,0,0,131,129,1,0,0,0,131,132,
  	1,0,0,0,132,15,1,0,0,0,133,134,5,40,0,0,134,135,5,52,0,0,135,17,1,0,0,
  	0,136,140,3,22,11,0,137,140,5,53,0,0,138,140,5,54,0,0,139,136,1,0,0,0,
  	139,137,1,0,0,0,139,138,1,0,0,0,140,19,1,0,0,0,141,142,5,31,0,0,142,144,
  	5,54,0,0,143,145,5,54,0,0,144,143,1,0,0,0,144,145,1,0,0,0,145,21,1,0,
  	0,0,146,147,5,54,0,0,147,148,5,76,0,0,148,149,5,54,0,0,149,23,1,0,0,0,
  	150,151,5,51,0,0,151,155,3,26,13,0,152,153,5,1,0,0,153,154,5,54,0,0,154,
  	156,5,2,0,0,155,152,1,0,0,0,155,156,1,0,0,0,156,25,1,0,0,0,157,164,5,
  	21,0,0,158,164,5,24,0,0,159,164,5,23,0,0,160,164,5,25,0,0,161,164,5,26,
  	0,0,162,164,5,22,0,0,163,157,1,0,0,0,163,158,1,0,0,0,163,159,1,0,0,0,
  	163,160,1,0,0,0,163,161,1,0,0,0,163,162,1,0,0,0,164,27,1,0,0,0,165,175,
  	3,34,17,0,166,171,3,36,18,0,167,168,5,71,0,0,168,170,3,36,18,0,169,167,
  	1,0,0,0,170,173,1,0,0,0,171,169,1,0,0,0,171,172,1,0,0,0,172,175,1,0,0,
  	0,173,171,1,0,0,0,174,165,1,0,0,0,174,166,1,0,0,0,175,29,1,0,0,0,176,
  	189,5,51,0,0,177,178,5,51,0,0,178,179,5,1,0,0,179,180,5,65,0,0,180,189,
  	5,2,0,0,181,182,5,51,0,0,182,183,5,64,0,0,183,189,5,51,0,0,184,185,5,
  	51,0,0,185,186,5,1,0,0,186,187,5,54,0,0,187,189,5,2,0,0,188,176,1,0,0,
  	0,188,177,1,0,0,0,188,181,1,0,0,0,188,184,1,0,0,0,189,31,1,0,0,0,190,
  	191,5,79,0,0,191,195,3,36,18,0,192,193,7,0,0,0,193,195,3,36,18,0,194,
  	190,1,0,0,0,194,192,1,0,0,0,195,33,1,0,0,0,196,197,5,51,0,0,197,199,5,
  	64,0,0,198,196,1,0,0,0,198,199,1,0,0,0,199,200,1,0,0,0,200,201,5,75,0,
  	0,201,35,1,0,0,0,202,203,3,44,22,0,203,37,1,0,0,0,204,205,3,40,20,0,205,
  	39,1,0,0,0,206,207,6,20,-1,0,207,208,3,42,21,0,208,217,1,0,0,0,209,210,
  	10,3,0,0,210,211,5,43,0,0,211,216,3,40,20,4,212,213,10,2,0,0,213,214,
  	5,44,0,0,214,216,3,40,20,3,215,209,1,0,0,0,215,212,1,0,0,0,216,219,1,
  	0,0,0,217,215,1,0,0,0,217,218,1,0,0,0,218,41,1,0,0,0,219,217,1,0,0,0,
  	220,221,6,21,-1,0,221,222,3,44,22,0,222,243,1,0,0,0,223,224,10,7,0,0,
  	224,225,5,56,0,0,225,242,3,42,21,8,226,227,10,6,0,0,227,228,5,57,0,0,
  	228,242,3,42,21,7,229,230,10,5,0,0,230,231,5,58,0,0,231,242,3,42,21,6,
  	232,233,10,4,0,0,233,234,5,59,0,0,234,242,3,42,21,5,235,236,10,3,0,0,
  	236,237,5,60,0,0,237,242,3,42,21,4,238,239,10,2,0,0,239,240,5,61,0,0,
  	240,242,3,42,21,3,241,223,1,0,0,0,241,226,1,0,0,0,241,229,1,0,0,0,241,
  	232,1,0,0,0,241,235,1,0,0,0,241,238,1,0,0,0,242,245,1,0,0,0,243,241,1,
  	0,0,0,243,244,1,0,0,0,244,43,1,0,0,0,245,243,1,0,0,0,246,247,6,22,-1,
  	0,247,248,3,46,23,0,248,257,1,0,0,0,249,250,10,3,0,0,250,251,5,77,0,0,
  	251,256,3,44,22,4,252,253,10,2,0,0,253,254,5,78,0,0,254,256,3,44,22,3,
  	255,249,1,0,0,0,255,252,1,0,0,0,256,259,1,0,0,0,257,255,1,0,0,0,257,258,
  	1,0,0,0,258,45,1,0,0,0,259,257,1,0,0,0,260,261,6,23,-1,0,261,282,3,22,
  	11,0,262,263,5,3,0,0,263,264,3,44,22,0,264,265,5,4,0,0,265,282,1,0,0,
  	0,266,268,5,78,0,0,267,266,1,0,0,0,267,268,1,0,0,0,268,269,1,0,0,0,269,
  	282,5,53,0,0,270,272,5,78,0,0,271,270,1,0,0,0,271,272,1,0,0,0,272,273,
  	1,0,0,0,273,282,5,54,0,0,274,282,5,52,0,0,275,282,3,32,16,0,276,282,3,
  	30,15,0,277,282,3,54,27,0,278,282,3,56,28,0,279,280,5,45,0,0,280,282,
  	3,46,23,1,281,260,1,0,0,0,281,262,1,0,0,0,281,267,1,0,0,0,281,271,1,0,
  	0,0,281,274,1,0,0,0,281,275,1,0,0,0,281,276,1,0,0,0,281,277,1,0,0,0,281,
  	278,1,0,0,0,281,279,1,0,0,0,282,291,1,0,0,0,283,284,10,12,0,0,284,285,
  	5,75,0,0,285,290,3,46,23,13,286,287,10,11,0,0,287,288,5,76,0,0,288,290,
  	3,46,23,12,289,283,1,0,0,0,289,286,1,0,0,0,290,293,1,0,0,0,291,289,1,
  	0,0,0,291,292,1,0,0,0,292,47,1,0,0,0,293,291,1,0,0,0,294,295,3,50,25,
  	0,295,296,5,58,0,0,296,297,5,54,0,0,297,308,1,0,0,0,298,299,3,50,25,0,
  	299,300,5,78,0,0,300,301,3,18,9,0,301,308,1,0,0,0,302,303,3,50,25,0,303,
  	304,5,77,0,0,304,305,3,50,25,0,305,308,1,0,0,0,306,308,3,50,25,0,307,
  	294,1,0,0,0,307,298,1,0,0,0,307,302,1,0,0,0,307,306,1,0,0,0,308,49,1,
  	0,0,0,309,310,3,52,26,0,310,311,5,67,0,0,311,312,3,52,26,0,312,338,1,
  	0,0,0,313,314,3,52,26,0,314,315,5,68,0,0,315,316,3,18,9,0,316,338,1,0,
  	0,0,317,318,3,52,26,0,318,319,5,69,0,0,319,320,3,18,9,0,320,338,1,0,0,
  	0,321,322,3,52,26,0,322,323,5,66,0,0,323,324,5,3,0,0,324,325,5,54,0,0,
  	325,327,5,71,0,0,326,328,5,78,0,0,327,326,1,0,0,0,327,328,1,0,0,0,328,
  	329,1,0,0,0,329,330,5,54,0,0,330,331,5,4,0,0,331,338,1,0,0,0,332,333,
  	3,52,26,0,333,334,5,64,0,0,334,335,3,54,27,0,335,338,1,0,0,0,336,338,
  	3,52,26,0,337,309,1,0,0,0,337,313,1,0,0,0,337,317,1,0,0,0,337,321,1,0,
  	0,0,337,332,1,0,0,0,337,336,1,0,0,0,338,51,1,0,0,0,339,345,5,51,0,0,340,
  	341,5,3,0,0,341,342,3,48,24,0,342,343,5,4,0,0,343,345,1,0,0,0,344,339,
  	1,0,0,0,344,340,1,0,0,0,345,53,1,0,0,0,346,351,5,46,0,0,347,351,5,47,
  	0,0,348,351,5,48,0,0,349,351,5,49,0,0,350,346,1,0,0,0,350,347,1,0,0,0,
  	350,348,1,0,0,0,350,349,1,0,0,0,351,55,1,0,0,0,352,353,7,1,0,0,353,354,
  	5,3,0,0,354,359,3,44,22,0,355,356,5,71,0,0,356,358,3,44,22,0,357,355,
  	1,0,0,0,358,361,1,0,0,0,359,357,1,0,0,0,359,360,1,0,0,0,360,362,1,0,0,
  	0,361,359,1,0,0,0,362,363,5,4,0,0,363,57,1,0,0,0,35,64,66,87,90,98,117,
  	121,126,131,139,144,155,163,171,174,188,194,198,215,217,241,243,255,257,
  	267,271,281,289,291,307,327,337,344,350,359
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  rqlParserStaticData = staticData.release();
}

}

RQLParser::RQLParser(TokenStream *input) : RQLParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

RQLParser::RQLParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  RQLParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *rqlParserStaticData->atn, rqlParserStaticData->decisionToDFA, rqlParserStaticData->sharedContextCache, options);
}

RQLParser::~RQLParser() {
  delete _interpreter;
}

const atn::ATN& RQLParser::getATN() const {
  return *rqlParserStaticData->atn;
}

std::string RQLParser::getGrammarFileName() const {
  return "RQL.g4";
}

const std::vector<std::string>& RQLParser::getRuleNames() const {
  return rqlParserStaticData->ruleNames;
}

const dfa::Vocabulary& RQLParser::getVocabulary() const {
  return rqlParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView RQLParser::getSerializedATN() const {
  return rqlParserStaticData->serializedATN;
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

std::vector<RQLParser::Storage_statementContext *> RQLParser::ProgContext::storage_statement() {
  return getRuleContexts<RQLParser::Storage_statementContext>();
}

RQLParser::Storage_statementContext* RQLParser::ProgContext::storage_statement(size_t i) {
  return getRuleContext<RQLParser::Storage_statementContext>(i);
}

std::vector<RQLParser::Substrat_statementContext *> RQLParser::ProgContext::substrat_statement() {
  return getRuleContexts<RQLParser::Substrat_statementContext>();
}

RQLParser::Substrat_statementContext* RQLParser::ProgContext::substrat_statement(size_t i) {
  return getRuleContext<RQLParser::Substrat_statementContext>(i);
}

std::vector<RQLParser::Compiler_optionContext *> RQLParser::ProgContext::compiler_option() {
  return getRuleContexts<RQLParser::Compiler_optionContext>();
}

RQLParser::Compiler_optionContext* RQLParser::ProgContext::compiler_option(size_t i) {
  return getRuleContext<RQLParser::Compiler_optionContext>(i);
}

std::vector<RQLParser::Rule_statementContext *> RQLParser::ProgContext::rule_statement() {
  return getRuleContexts<RQLParser::Rule_statementContext>();
}

RQLParser::Rule_statementContext* RQLParser::ProgContext::rule_statement(size_t i) {
  return getRuleContext<RQLParser::Rule_statementContext>(i);
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
    setState(64); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(64);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::SELECT: {
          setState(58);
          select_statement();
          break;
        }

        case RQLParser::DECLARE: {
          setState(59);
          declare_statement();
          break;
        }

        case RQLParser::STORAGE: {
          setState(60);
          storage_statement();
          break;
        }

        case RQLParser::SUBSTRAT: {
          setState(61);
          substrat_statement();
          break;
        }

        case RQLParser::COPTION: {
          setState(62);
          compiler_option();
          break;
        }

        case RQLParser::RULE: {
          setState(63);
          rule_statement();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(66); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 130056978432) != 0));
    setState(68);
    match(RQLParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Compiler_optionContext ------------------------------------------------------------------

RQLParser::Compiler_optionContext::Compiler_optionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Compiler_optionContext::getRuleIndex() const {
  return RQLParser::RuleCompiler_option;
}

void RQLParser::Compiler_optionContext::copyFrom(Compiler_optionContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- CoptionContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::CoptionContext::COPTION() {
  return getToken(RQLParser::COPTION, 0);
}

tree::TerminalNode* RQLParser::CoptionContext::STRING() {
  return getToken(RQLParser::STRING, 0);
}

RQLParser::CoptionContext::CoptionContext(Compiler_optionContext *ctx) { copyFrom(ctx); }

void RQLParser::CoptionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCoption(this);
}
void RQLParser::CoptionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCoption(this);
}
RQLParser::Compiler_optionContext* RQLParser::compiler_option() {
  Compiler_optionContext *_localctx = _tracker.createInstance<Compiler_optionContext>(_ctx, getState());
  enterRule(_localctx, 2, RQLParser::RuleCompiler_option);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::CoptionContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(70);
    match(RQLParser::COPTION);
    setState(71);
    antlrcpp::downCast<CoptionContext *>(_localctx)->compiler_options = match(RQLParser::STRING);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Storage_statementContext ------------------------------------------------------------------

RQLParser::Storage_statementContext::Storage_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Storage_statementContext::getRuleIndex() const {
  return RQLParser::RuleStorage_statement;
}

void RQLParser::Storage_statementContext::copyFrom(Storage_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- StorageContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::StorageContext::STORAGE() {
  return getToken(RQLParser::STORAGE, 0);
}

tree::TerminalNode* RQLParser::StorageContext::STRING() {
  return getToken(RQLParser::STRING, 0);
}

RQLParser::StorageContext::StorageContext(Storage_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::StorageContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStorage(this);
}
void RQLParser::StorageContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStorage(this);
}
RQLParser::Storage_statementContext* RQLParser::storage_statement() {
  Storage_statementContext *_localctx = _tracker.createInstance<Storage_statementContext>(_ctx, getState());
  enterRule(_localctx, 4, RQLParser::RuleStorage_statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::StorageContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(73);
    match(RQLParser::STORAGE);
    setState(74);
    antlrcpp::downCast<StorageContext *>(_localctx)->folder_name = match(RQLParser::STRING);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Substrat_statementContext ------------------------------------------------------------------

RQLParser::Substrat_statementContext::Substrat_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Substrat_statementContext::getRuleIndex() const {
  return RQLParser::RuleSubstrat_statement;
}

void RQLParser::Substrat_statementContext::copyFrom(Substrat_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- SubstratContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::SubstratContext::SUBSTRAT() {
  return getToken(RQLParser::SUBSTRAT, 0);
}

tree::TerminalNode* RQLParser::SubstratContext::STRING_SUBSTRAT() {
  return getToken(RQLParser::STRING_SUBSTRAT, 0);
}

RQLParser::SubstratContext::SubstratContext(Substrat_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::SubstratContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSubstrat(this);
}
void RQLParser::SubstratContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSubstrat(this);
}
RQLParser::Substrat_statementContext* RQLParser::substrat_statement() {
  Substrat_statementContext *_localctx = _tracker.createInstance<Substrat_statementContext>(_ctx, getState());
  enterRule(_localctx, 6, RQLParser::RuleSubstrat_statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::SubstratContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(76);
    match(RQLParser::SUBSTRAT);
    setState(77);
    antlrcpp::downCast<SubstratContext *>(_localctx)->substrat_type = match(RQLParser::STRING_SUBSTRAT);
   
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

tree::TerminalNode* RQLParser::SelectContext::FROM() {
  return getToken(RQLParser::FROM, 0);
}

RQLParser::Stream_expressionContext* RQLParser::SelectContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

tree::TerminalNode* RQLParser::SelectContext::ID() {
  return getToken(RQLParser::ID, 0);
}

tree::TerminalNode* RQLParser::SelectContext::FILE() {
  return getToken(RQLParser::FILE, 0);
}

RQLParser::Retention_fromContext* RQLParser::SelectContext::retention_from() {
  return getRuleContext<RQLParser::Retention_fromContext>(0);
}

tree::TerminalNode* RQLParser::SelectContext::STRING() {
  return getToken(RQLParser::STRING, 0);
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
  enterRule(_localctx, 8, RQLParser::RuleSelect_statement);
  size_t _la = 0;

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
    setState(79);
    match(RQLParser::SELECT);
    setState(80);
    select_list();
    setState(81);
    match(RQLParser::STREAM);
    setState(82);
    antlrcpp::downCast<SelectContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(83);
    match(RQLParser::FROM);
    setState(84);
    stream_expression();
    setState(87);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::FILE) {
      setState(85);
      match(RQLParser::FILE);
      setState(86);
      antlrcpp::downCast<SelectContext *>(_localctx)->name = match(RQLParser::STRING);
    }
    setState(90);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::RETENTION) {
      setState(89);
      retention_from();
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

std::vector<RQLParser::Field_declarationContext *> RQLParser::DeclareContext::field_declaration() {
  return getRuleContexts<RQLParser::Field_declarationContext>();
}

RQLParser::Field_declarationContext* RQLParser::DeclareContext::field_declaration(size_t i) {
  return getRuleContext<RQLParser::Field_declarationContext>(i);
}

tree::TerminalNode* RQLParser::DeclareContext::STREAM() {
  return getToken(RQLParser::STREAM, 0);
}

std::vector<tree::TerminalNode *> RQLParser::DeclareContext::COMMA() {
  return getTokens(RQLParser::COMMA);
}

tree::TerminalNode* RQLParser::DeclareContext::COMMA(size_t i) {
  return getToken(RQLParser::COMMA, i);
}

RQLParser::Rational_seContext* RQLParser::DeclareContext::rational_se() {
  return getRuleContext<RQLParser::Rational_seContext>(0);
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
  enterRule(_localctx, 10, RQLParser::RuleDeclare_statement);
  size_t _la = 0;

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
    setState(92);
    match(RQLParser::DECLARE);
    setState(93);
    field_declaration();
    setState(98);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(94);
      match(RQLParser::COMMA);
      setState(95);
      field_declaration();
      setState(100);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(101);
    match(RQLParser::STREAM);
    setState(102);
    antlrcpp::downCast<DeclareContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(103);
    match(RQLParser::COMMA);
    setState(104);
    rational_se();
    setState(105);
    match(RQLParser::FILE);
    setState(106);
    antlrcpp::downCast<DeclareContext *>(_localctx)->file_name = match(RQLParser::STRING);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Rule_statementContext ------------------------------------------------------------------

RQLParser::Rule_statementContext::Rule_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Rule_statementContext::getRuleIndex() const {
  return RQLParser::RuleRule_statement;
}

void RQLParser::Rule_statementContext::copyFrom(Rule_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- RulezContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::RulezContext::RULE() {
  return getToken(RQLParser::RULE, 0);
}

tree::TerminalNode* RQLParser::RulezContext::ON() {
  return getToken(RQLParser::ON, 0);
}

tree::TerminalNode* RQLParser::RulezContext::WHEN() {
  return getToken(RQLParser::WHEN, 0);
}

RQLParser::LogicContext* RQLParser::RulezContext::logic() {
  return getRuleContext<RQLParser::LogicContext>(0);
}

tree::TerminalNode* RQLParser::RulezContext::DO() {
  return getToken(RQLParser::DO, 0);
}

std::vector<tree::TerminalNode *> RQLParser::RulezContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::RulezContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

RQLParser::DumppartContext* RQLParser::RulezContext::dumppart() {
  return getRuleContext<RQLParser::DumppartContext>(0);
}

RQLParser::SystempartContext* RQLParser::RulezContext::systempart() {
  return getRuleContext<RQLParser::SystempartContext>(0);
}

RQLParser::RulezContext::RulezContext(Rule_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::RulezContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRulez(this);
}
void RQLParser::RulezContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRulez(this);
}
RQLParser::Rule_statementContext* RQLParser::rule_statement() {
  Rule_statementContext *_localctx = _tracker.createInstance<Rule_statementContext>(_ctx, getState());
  enterRule(_localctx, 12, RQLParser::RuleRule_statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::RulezContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(108);
    match(RQLParser::RULE);
    setState(109);
    antlrcpp::downCast<RulezContext *>(_localctx)->name = match(RQLParser::ID);
    setState(110);
    match(RQLParser::ON);
    setState(111);
    antlrcpp::downCast<RulezContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(112);
    match(RQLParser::WHEN);
    setState(113);
    logic();
    setState(114);
    match(RQLParser::DO);
    setState(117);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::DUMP: {
        setState(115);
        dumppart();
        break;
      }

      case RQLParser::SYSTEM: {
        setState(116);
        systempart();
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

//----------------- DumppartContext ------------------------------------------------------------------

RQLParser::DumppartContext::DumppartContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::DumppartContext::DUMP() {
  return getToken(RQLParser::DUMP, 0);
}

tree::TerminalNode* RQLParser::DumppartContext::TO() {
  return getToken(RQLParser::TO, 0);
}

std::vector<tree::TerminalNode *> RQLParser::DumppartContext::DECIMAL() {
  return getTokens(RQLParser::DECIMAL);
}

tree::TerminalNode* RQLParser::DumppartContext::DECIMAL(size_t i) {
  return getToken(RQLParser::DECIMAL, i);
}

std::vector<tree::TerminalNode *> RQLParser::DumppartContext::MINUS() {
  return getTokens(RQLParser::MINUS);
}

tree::TerminalNode* RQLParser::DumppartContext::MINUS(size_t i) {
  return getToken(RQLParser::MINUS, i);
}

tree::TerminalNode* RQLParser::DumppartContext::RETENTION() {
  return getToken(RQLParser::RETENTION, 0);
}


size_t RQLParser::DumppartContext::getRuleIndex() const {
  return RQLParser::RuleDumppart;
}

void RQLParser::DumppartContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDumppart(this);
}

void RQLParser::DumppartContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDumppart(this);
}

RQLParser::DumppartContext* RQLParser::dumppart() {
  DumppartContext *_localctx = _tracker.createInstance<DumppartContext>(_ctx, getState());
  enterRule(_localctx, 14, RQLParser::RuleDumppart);
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
    setState(119);
    match(RQLParser::DUMP);
    setState(121);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::MINUS) {
      setState(120);
      match(RQLParser::MINUS);
    }
    setState(123);
    antlrcpp::downCast<DumppartContext *>(_localctx)->step_back = match(RQLParser::DECIMAL);
    setState(124);
    match(RQLParser::TO);
    setState(126);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::MINUS) {
      setState(125);
      match(RQLParser::MINUS);
    }
    setState(128);
    antlrcpp::downCast<DumppartContext *>(_localctx)->step_forward = match(RQLParser::DECIMAL);
    setState(131);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::RETENTION) {
      setState(129);
      match(RQLParser::RETENTION);
      setState(130);
      antlrcpp::downCast<DumppartContext *>(_localctx)->rule_retnetion = match(RQLParser::DECIMAL);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SystempartContext ------------------------------------------------------------------

RQLParser::SystempartContext::SystempartContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::SystempartContext::SYSTEM() {
  return getToken(RQLParser::SYSTEM, 0);
}

tree::TerminalNode* RQLParser::SystempartContext::STRING() {
  return getToken(RQLParser::STRING, 0);
}


size_t RQLParser::SystempartContext::getRuleIndex() const {
  return RQLParser::RuleSystempart;
}

void RQLParser::SystempartContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSystempart(this);
}

void RQLParser::SystempartContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSystempart(this);
}

RQLParser::SystempartContext* RQLParser::systempart() {
  SystempartContext *_localctx = _tracker.createInstance<SystempartContext>(_ctx, getState());
  enterRule(_localctx, 16, RQLParser::RuleSystempart);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(133);
    match(RQLParser::SYSTEM);
    setState(134);
    antlrcpp::downCast<SystempartContext *>(_localctx)->syscmd = match(RQLParser::STRING);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Rational_seContext ------------------------------------------------------------------

RQLParser::Rational_seContext::Rational_seContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Rational_seContext::getRuleIndex() const {
  return RQLParser::RuleRational_se;
}

void RQLParser::Rational_seContext::copyFrom(Rational_seContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- RationalAsDecimalContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::RationalAsDecimalContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

RQLParser::RationalAsDecimalContext::RationalAsDecimalContext(Rational_seContext *ctx) { copyFrom(ctx); }

void RQLParser::RationalAsDecimalContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRationalAsDecimal(this);
}
void RQLParser::RationalAsDecimalContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRationalAsDecimal(this);
}
//----------------- RationalAsFloatContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::RationalAsFloatContext::FLOAT() {
  return getToken(RQLParser::FLOAT, 0);
}

RQLParser::RationalAsFloatContext::RationalAsFloatContext(Rational_seContext *ctx) { copyFrom(ctx); }

void RQLParser::RationalAsFloatContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRationalAsFloat(this);
}
void RQLParser::RationalAsFloatContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRationalAsFloat(this);
}
//----------------- RationalAsFraction_proformaContext ------------------------------------------------------------------

RQLParser::Fraction_ruleContext* RQLParser::RationalAsFraction_proformaContext::fraction_rule() {
  return getRuleContext<RQLParser::Fraction_ruleContext>(0);
}

RQLParser::RationalAsFraction_proformaContext::RationalAsFraction_proformaContext(Rational_seContext *ctx) { copyFrom(ctx); }

void RQLParser::RationalAsFraction_proformaContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRationalAsFraction_proforma(this);
}
void RQLParser::RationalAsFraction_proformaContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRationalAsFraction_proforma(this);
}
RQLParser::Rational_seContext* RQLParser::rational_se() {
  Rational_seContext *_localctx = _tracker.createInstance<Rational_seContext>(_ctx, getState());
  enterRule(_localctx, 18, RQLParser::RuleRational_se);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(139);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsFraction_proformaContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(136);
      fraction_rule();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsFloatContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(137);
      match(RQLParser::FLOAT);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsDecimalContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(138);
      match(RQLParser::DECIMAL);
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

//----------------- Retention_fromContext ------------------------------------------------------------------

RQLParser::Retention_fromContext::Retention_fromContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Retention_fromContext::getRuleIndex() const {
  return RQLParser::RuleRetention_from;
}

void RQLParser::Retention_fromContext::copyFrom(Retention_fromContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- RetentionContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::RetentionContext::RETENTION() {
  return getToken(RQLParser::RETENTION, 0);
}

std::vector<tree::TerminalNode *> RQLParser::RetentionContext::DECIMAL() {
  return getTokens(RQLParser::DECIMAL);
}

tree::TerminalNode* RQLParser::RetentionContext::DECIMAL(size_t i) {
  return getToken(RQLParser::DECIMAL, i);
}

RQLParser::RetentionContext::RetentionContext(Retention_fromContext *ctx) { copyFrom(ctx); }

void RQLParser::RetentionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRetention(this);
}
void RQLParser::RetentionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRetention(this);
}
RQLParser::Retention_fromContext* RQLParser::retention_from() {
  Retention_fromContext *_localctx = _tracker.createInstance<Retention_fromContext>(_ctx, getState());
  enterRule(_localctx, 20, RQLParser::RuleRetention_from);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::RetentionContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(141);
    match(RQLParser::RETENTION);
    setState(142);
    antlrcpp::downCast<RetentionContext *>(_localctx)->capacity = match(RQLParser::DECIMAL);
    setState(144);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::DECIMAL) {
      setState(143);
      antlrcpp::downCast<RetentionContext *>(_localctx)->segments = match(RQLParser::DECIMAL);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Fraction_ruleContext ------------------------------------------------------------------

RQLParser::Fraction_ruleContext::Fraction_ruleContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Fraction_ruleContext::getRuleIndex() const {
  return RQLParser::RuleFraction_rule;
}

void RQLParser::Fraction_ruleContext::copyFrom(Fraction_ruleContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- FractionContext ------------------------------------------------------------------

std::vector<tree::TerminalNode *> RQLParser::FractionContext::DECIMAL() {
  return getTokens(RQLParser::DECIMAL);
}

tree::TerminalNode* RQLParser::FractionContext::DECIMAL(size_t i) {
  return getToken(RQLParser::DECIMAL, i);
}

tree::TerminalNode* RQLParser::FractionContext::DIVIDE() {
  return getToken(RQLParser::DIVIDE, 0);
}

RQLParser::FractionContext::FractionContext(Fraction_ruleContext *ctx) { copyFrom(ctx); }

void RQLParser::FractionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFraction(this);
}
void RQLParser::FractionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFraction(this);
}
RQLParser::Fraction_ruleContext* RQLParser::fraction_rule() {
  Fraction_ruleContext *_localctx = _tracker.createInstance<Fraction_ruleContext>(_ctx, getState());
  enterRule(_localctx, 22, RQLParser::RuleFraction_rule);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::FractionContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(146);
    match(RQLParser::DECIMAL);
    setState(147);
    match(RQLParser::DIVIDE);
    setState(148);
    match(RQLParser::DECIMAL);
   
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

tree::TerminalNode* RQLParser::SingleDeclarationContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
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
  enterRule(_localctx, 24, RQLParser::RuleField_declaration);
  size_t _la = 0;

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
    setState(150);
    match(RQLParser::ID);
    setState(151);
    field_type();
    setState(155);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::T__0) {
      setState(152);
      match(RQLParser::T__0);
      setState(153);
      antlrcpp::downCast<SingleDeclarationContext *>(_localctx)->type_size = match(RQLParser::DECIMAL);
      setState(154);
      match(RQLParser::T__1);
    }
   
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


size_t RQLParser::Field_typeContext::getRuleIndex() const {
  return RQLParser::RuleField_type;
}

void RQLParser::Field_typeContext::copyFrom(Field_typeContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- TypeUnsignedContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::TypeUnsignedContext::UNSIGNED_T() {
  return getToken(RQLParser::UNSIGNED_T, 0);
}

RQLParser::TypeUnsignedContext::TypeUnsignedContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeUnsignedContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTypeUnsigned(this);
}
void RQLParser::TypeUnsignedContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTypeUnsigned(this);
}
//----------------- TypeIntContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::TypeIntContext::INTEGER_T() {
  return getToken(RQLParser::INTEGER_T, 0);
}

RQLParser::TypeIntContext::TypeIntContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeIntContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTypeInt(this);
}
void RQLParser::TypeIntContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTypeInt(this);
}
//----------------- TypeFloatContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::TypeFloatContext::FLOAT_T() {
  return getToken(RQLParser::FLOAT_T, 0);
}

RQLParser::TypeFloatContext::TypeFloatContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeFloatContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTypeFloat(this);
}
void RQLParser::TypeFloatContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTypeFloat(this);
}
//----------------- TypeStringContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::TypeStringContext::STRING_T() {
  return getToken(RQLParser::STRING_T, 0);
}

RQLParser::TypeStringContext::TypeStringContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeStringContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTypeString(this);
}
void RQLParser::TypeStringContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTypeString(this);
}
//----------------- TypeByteContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::TypeByteContext::BYTE_T() {
  return getToken(RQLParser::BYTE_T, 0);
}

RQLParser::TypeByteContext::TypeByteContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeByteContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTypeByte(this);
}
void RQLParser::TypeByteContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTypeByte(this);
}
//----------------- TypeDoubleContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::TypeDoubleContext::DOUBLE_T() {
  return getToken(RQLParser::DOUBLE_T, 0);
}

RQLParser::TypeDoubleContext::TypeDoubleContext(Field_typeContext *ctx) { copyFrom(ctx); }

void RQLParser::TypeDoubleContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTypeDouble(this);
}
void RQLParser::TypeDoubleContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTypeDouble(this);
}
RQLParser::Field_typeContext* RQLParser::field_type() {
  Field_typeContext *_localctx = _tracker.createInstance<Field_typeContext>(_ctx, getState());
  enterRule(_localctx, 26, RQLParser::RuleField_type);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(163);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::BYTE_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeByteContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(157);
        match(RQLParser::BYTE_T);
        break;
      }

      case RQLParser::INTEGER_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeIntContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(158);
        match(RQLParser::INTEGER_T);
        break;
      }

      case RQLParser::UNSIGNED_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeUnsignedContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(159);
        match(RQLParser::UNSIGNED_T);
        break;
      }

      case RQLParser::FLOAT_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeFloatContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(160);
        match(RQLParser::FLOAT_T);
        break;
      }

      case RQLParser::DOUBLE_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeDoubleContext>(_localctx);
        enterOuterAlt(_localctx, 5);
        setState(161);
        match(RQLParser::DOUBLE_T);
        break;
      }

      case RQLParser::STRING_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeStringContext>(_localctx);
        enterOuterAlt(_localctx, 6);
        setState(162);
        match(RQLParser::STRING_T);
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


size_t RQLParser::Select_listContext::getRuleIndex() const {
  return RQLParser::RuleSelect_list;
}

void RQLParser::Select_listContext::copyFrom(Select_listContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- SelectListContext ------------------------------------------------------------------

std::vector<RQLParser::ExpressionContext *> RQLParser::SelectListContext::expression() {
  return getRuleContexts<RQLParser::ExpressionContext>();
}

RQLParser::ExpressionContext* RQLParser::SelectListContext::expression(size_t i) {
  return getRuleContext<RQLParser::ExpressionContext>(i);
}

std::vector<tree::TerminalNode *> RQLParser::SelectListContext::COMMA() {
  return getTokens(RQLParser::COMMA);
}

tree::TerminalNode* RQLParser::SelectListContext::COMMA(size_t i) {
  return getToken(RQLParser::COMMA, i);
}

RQLParser::SelectListContext::SelectListContext(Select_listContext *ctx) { copyFrom(ctx); }

void RQLParser::SelectListContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSelectList(this);
}
void RQLParser::SelectListContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSelectList(this);
}
//----------------- SelectListFullscanContext ------------------------------------------------------------------

RQLParser::AsteriskContext* RQLParser::SelectListFullscanContext::asterisk() {
  return getRuleContext<RQLParser::AsteriskContext>(0);
}

RQLParser::SelectListFullscanContext::SelectListFullscanContext(Select_listContext *ctx) { copyFrom(ctx); }

void RQLParser::SelectListFullscanContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSelectListFullscan(this);
}
void RQLParser::SelectListFullscanContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSelectListFullscan(this);
}
RQLParser::Select_listContext* RQLParser::select_list() {
  Select_listContext *_localctx = _tracker.createInstance<Select_listContext>(_ctx, getState());
  enterRule(_localctx, 28, RQLParser::RuleSelect_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(174);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 14, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::SelectListFullscanContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(165);
      asterisk();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::SelectListContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(166);
      expression();
      setState(171);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == RQLParser::COMMA) {
        setState(167);
        match(RQLParser::COMMA);
        setState(168);
        expression();
        setState(173);
        _errHandler->sync(this);
        _la = _input->LA(1);
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
//----------------- FieldIDColumnNameContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::FieldIDColumnNameContext::DOT() {
  return getToken(RQLParser::DOT, 0);
}

std::vector<tree::TerminalNode *> RQLParser::FieldIDColumnNameContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::FieldIDColumnNameContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

RQLParser::FieldIDColumnNameContext::FieldIDColumnNameContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDColumnNameContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFieldIDColumnName(this);
}
void RQLParser::FieldIDColumnNameContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFieldIDColumnName(this);
}
RQLParser::Field_idContext* RQLParser::field_id() {
  Field_idContext *_localctx = _tracker.createInstance<Field_idContext>(_ctx, getState());
  enterRule(_localctx, 30, RQLParser::RuleField_id);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(188);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 15, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(176);
      antlrcpp::downCast<FieldIDContext *>(_localctx)->column_name = match(RQLParser::ID);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDUnderlineContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(177);
      antlrcpp::downCast<FieldIDUnderlineContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(178);
      match(RQLParser::T__0);
      setState(179);
      match(RQLParser::UNDERLINE);
      setState(180);
      match(RQLParser::T__1);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDColumnNameContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(181);
      antlrcpp::downCast<FieldIDColumnNameContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(182);
      match(RQLParser::DOT);
      setState(183);
      antlrcpp::downCast<FieldIDColumnNameContext *>(_localctx)->column_name = match(RQLParser::ID);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDTableContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(184);
      antlrcpp::downCast<FieldIDTableContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(185);
      match(RQLParser::T__0);
      setState(186);
      antlrcpp::downCast<FieldIDTableContext *>(_localctx)->column_index = match(RQLParser::DECIMAL);
      setState(187);
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
  enterRule(_localctx, 32, RQLParser::RuleUnary_op_expression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(194);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::BIT_NOT: {
        enterOuterAlt(_localctx, 1);
        setState(190);
        match(RQLParser::BIT_NOT);
        setState(191);
        expression();
        break;
      }

      case RQLParser::PLUS:
      case RQLParser::MINUS: {
        enterOuterAlt(_localctx, 2);
        setState(192);
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
        setState(193);
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
  enterRule(_localctx, 34, RQLParser::RuleAsterisk);
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
    setState(198);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::ID) {
      setState(196);
      match(RQLParser::ID);
      setState(197);
      match(RQLParser::DOT);
    }
    setState(200);
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

RQLParser::Expression_factorContext* RQLParser::ExpressionContext::expression_factor() {
  return getRuleContext<RQLParser::Expression_factorContext>(0);
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
  ExpressionContext *_localctx = _tracker.createInstance<ExpressionContext>(_ctx, getState());
  enterRule(_localctx, 36, RQLParser::RuleExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(202);
    expression_factor(0);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LogicContext ------------------------------------------------------------------

RQLParser::LogicContext::LogicContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::LogicContext::getRuleIndex() const {
  return RQLParser::RuleLogic;
}

void RQLParser::LogicContext::copyFrom(LogicContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- LogicExpressionContext ------------------------------------------------------------------

RQLParser::Expression_logicContext* RQLParser::LogicExpressionContext::expression_logic() {
  return getRuleContext<RQLParser::Expression_logicContext>(0);
}

RQLParser::LogicExpressionContext::LogicExpressionContext(LogicContext *ctx) { copyFrom(ctx); }

void RQLParser::LogicExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLogicExpression(this);
}
void RQLParser::LogicExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLogicExpression(this);
}
RQLParser::LogicContext* RQLParser::logic() {
  LogicContext *_localctx = _tracker.createInstance<LogicContext>(_ctx, getState());
  enterRule(_localctx, 38, RQLParser::RuleLogic);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::LogicExpressionContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(204);
    expression_logic(0);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Expression_logicContext ------------------------------------------------------------------

RQLParser::Expression_logicContext::Expression_logicContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Expression_logicContext::getRuleIndex() const {
  return RQLParser::RuleExpression_logic;
}

void RQLParser::Expression_logicContext::copyFrom(Expression_logicContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ExpOrContext ------------------------------------------------------------------

std::vector<RQLParser::Expression_logicContext *> RQLParser::ExpOrContext::expression_logic() {
  return getRuleContexts<RQLParser::Expression_logicContext>();
}

RQLParser::Expression_logicContext* RQLParser::ExpOrContext::expression_logic(size_t i) {
  return getRuleContext<RQLParser::Expression_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpOrContext::OR_C() {
  return getToken(RQLParser::OR_C, 0);
}

RQLParser::ExpOrContext::ExpOrContext(Expression_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpOrContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpOr(this);
}
void RQLParser::ExpOrContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpOr(this);
}
//----------------- ExpAndContext ------------------------------------------------------------------

std::vector<RQLParser::Expression_logicContext *> RQLParser::ExpAndContext::expression_logic() {
  return getRuleContexts<RQLParser::Expression_logicContext>();
}

RQLParser::Expression_logicContext* RQLParser::ExpAndContext::expression_logic(size_t i) {
  return getRuleContext<RQLParser::Expression_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpAndContext::AND_C() {
  return getToken(RQLParser::AND_C, 0);
}

RQLParser::ExpAndContext::ExpAndContext(Expression_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpAndContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpAnd(this);
}
void RQLParser::ExpAndContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpAnd(this);
}
//----------------- ExpTermLogicContext ------------------------------------------------------------------

RQLParser::Term_logicContext* RQLParser::ExpTermLogicContext::term_logic() {
  return getRuleContext<RQLParser::Term_logicContext>(0);
}

RQLParser::ExpTermLogicContext::ExpTermLogicContext(Expression_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpTermLogicContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpTermLogic(this);
}
void RQLParser::ExpTermLogicContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpTermLogic(this);
}

RQLParser::Expression_logicContext* RQLParser::expression_logic() {
   return expression_logic(0);
}

RQLParser::Expression_logicContext* RQLParser::expression_logic(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  RQLParser::Expression_logicContext *_localctx = _tracker.createInstance<Expression_logicContext>(_ctx, parentState);
  RQLParser::Expression_logicContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 40;
  enterRecursionRule(_localctx, 40, RQLParser::RuleExpression_logic, precedence);

    

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
    _localctx = _tracker.createInstance<ExpTermLogicContext>(_localctx);
    _ctx = _localctx;
    previousContext = _localctx;

    setState(207);
    term_logic(0);
    _ctx->stop = _input->LT(-1);
    setState(217);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 19, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(215);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 18, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpAndContext>(_tracker.createInstance<Expression_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_logic);
          setState(209);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(210);
          match(RQLParser::AND_C);
          setState(211);
          expression_logic(4);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpOrContext>(_tracker.createInstance<Expression_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_logic);
          setState(212);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(213);
          match(RQLParser::OR_C);
          setState(214);
          expression_logic(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(219);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 19, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- Term_logicContext ------------------------------------------------------------------

RQLParser::Term_logicContext::Term_logicContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Term_logicContext::getRuleIndex() const {
  return RQLParser::RuleTerm_logic;
}

void RQLParser::Term_logicContext::copyFrom(Term_logicContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ExpLsContext ------------------------------------------------------------------

std::vector<RQLParser::Term_logicContext *> RQLParser::ExpLsContext::term_logic() {
  return getRuleContexts<RQLParser::Term_logicContext>();
}

RQLParser::Term_logicContext* RQLParser::ExpLsContext::term_logic(size_t i) {
  return getRuleContext<RQLParser::Term_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpLsContext::IS_LS() {
  return getToken(RQLParser::IS_LS, 0);
}

RQLParser::ExpLsContext::ExpLsContext(Term_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpLsContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpLs(this);
}
void RQLParser::ExpLsContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpLs(this);
}
//----------------- ExpLeContext ------------------------------------------------------------------

std::vector<RQLParser::Term_logicContext *> RQLParser::ExpLeContext::term_logic() {
  return getRuleContexts<RQLParser::Term_logicContext>();
}

RQLParser::Term_logicContext* RQLParser::ExpLeContext::term_logic(size_t i) {
  return getRuleContext<RQLParser::Term_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpLeContext::IS_LE() {
  return getToken(RQLParser::IS_LE, 0);
}

RQLParser::ExpLeContext::ExpLeContext(Term_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpLeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpLe(this);
}
void RQLParser::ExpLeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpLe(this);
}
//----------------- ExpNqContext ------------------------------------------------------------------

std::vector<RQLParser::Term_logicContext *> RQLParser::ExpNqContext::term_logic() {
  return getRuleContexts<RQLParser::Term_logicContext>();
}

RQLParser::Term_logicContext* RQLParser::ExpNqContext::term_logic(size_t i) {
  return getRuleContext<RQLParser::Term_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpNqContext::IS_NQ() {
  return getToken(RQLParser::IS_NQ, 0);
}

RQLParser::ExpNqContext::ExpNqContext(Term_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpNqContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpNq(this);
}
void RQLParser::ExpNqContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpNq(this);
}
//----------------- ExpGrContext ------------------------------------------------------------------

std::vector<RQLParser::Term_logicContext *> RQLParser::ExpGrContext::term_logic() {
  return getRuleContexts<RQLParser::Term_logicContext>();
}

RQLParser::Term_logicContext* RQLParser::ExpGrContext::term_logic(size_t i) {
  return getRuleContext<RQLParser::Term_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpGrContext::IS_GR() {
  return getToken(RQLParser::IS_GR, 0);
}

RQLParser::ExpGrContext::ExpGrContext(Term_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpGrContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpGr(this);
}
void RQLParser::ExpGrContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpGr(this);
}
//----------------- ExpFactorContext ------------------------------------------------------------------

RQLParser::Expression_factorContext* RQLParser::ExpFactorContext::expression_factor() {
  return getRuleContext<RQLParser::Expression_factorContext>(0);
}

RQLParser::ExpFactorContext::ExpFactorContext(Term_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpFactorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpFactor(this);
}
void RQLParser::ExpFactorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpFactor(this);
}
//----------------- ExpEqContext ------------------------------------------------------------------

std::vector<RQLParser::Term_logicContext *> RQLParser::ExpEqContext::term_logic() {
  return getRuleContexts<RQLParser::Term_logicContext>();
}

RQLParser::Term_logicContext* RQLParser::ExpEqContext::term_logic(size_t i) {
  return getRuleContext<RQLParser::Term_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpEqContext::IS_EQ() {
  return getToken(RQLParser::IS_EQ, 0);
}

RQLParser::ExpEqContext::ExpEqContext(Term_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpEqContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpEq(this);
}
void RQLParser::ExpEqContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpEq(this);
}
//----------------- ExpGeContext ------------------------------------------------------------------

std::vector<RQLParser::Term_logicContext *> RQLParser::ExpGeContext::term_logic() {
  return getRuleContexts<RQLParser::Term_logicContext>();
}

RQLParser::Term_logicContext* RQLParser::ExpGeContext::term_logic(size_t i) {
  return getRuleContext<RQLParser::Term_logicContext>(i);
}

tree::TerminalNode* RQLParser::ExpGeContext::IS_GE() {
  return getToken(RQLParser::IS_GE, 0);
}

RQLParser::ExpGeContext::ExpGeContext(Term_logicContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpGeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpGe(this);
}
void RQLParser::ExpGeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpGe(this);
}

RQLParser::Term_logicContext* RQLParser::term_logic() {
   return term_logic(0);
}

RQLParser::Term_logicContext* RQLParser::term_logic(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  RQLParser::Term_logicContext *_localctx = _tracker.createInstance<Term_logicContext>(_ctx, parentState);
  RQLParser::Term_logicContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 42;
  enterRecursionRule(_localctx, 42, RQLParser::RuleTerm_logic, precedence);

    

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
    _localctx = _tracker.createInstance<ExpFactorContext>(_localctx);
    _ctx = _localctx;
    previousContext = _localctx;

    setState(221);
    expression_factor(0);
    _ctx->stop = _input->LT(-1);
    setState(243);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 21, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(241);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 20, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpEqContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(223);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(224);
          match(RQLParser::IS_EQ);
          setState(225);
          term_logic(8);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpNqContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(226);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(227);
          match(RQLParser::IS_NQ);
          setState(228);
          term_logic(7);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<ExpGrContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(229);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(230);
          match(RQLParser::IS_GR);
          setState(231);
          term_logic(6);
          break;
        }

        case 4: {
          auto newContext = _tracker.createInstance<ExpLsContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(232);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(233);
          match(RQLParser::IS_LS);
          setState(234);
          term_logic(5);
          break;
        }

        case 5: {
          auto newContext = _tracker.createInstance<ExpGeContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(235);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(236);
          match(RQLParser::IS_GE);
          setState(237);
          term_logic(4);
          break;
        }

        case 6: {
          auto newContext = _tracker.createInstance<ExpLeContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(238);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(239);
          match(RQLParser::IS_LE);
          setState(240);
          term_logic(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(245);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 21, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- Expression_factorContext ------------------------------------------------------------------

RQLParser::Expression_factorContext::Expression_factorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Expression_factorContext::getRuleIndex() const {
  return RQLParser::RuleExpression_factor;
}

void RQLParser::Expression_factorContext::copyFrom(Expression_factorContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ExpPlusContext ------------------------------------------------------------------

std::vector<RQLParser::Expression_factorContext *> RQLParser::ExpPlusContext::expression_factor() {
  return getRuleContexts<RQLParser::Expression_factorContext>();
}

RQLParser::Expression_factorContext* RQLParser::ExpPlusContext::expression_factor(size_t i) {
  return getRuleContext<RQLParser::Expression_factorContext>(i);
}

tree::TerminalNode* RQLParser::ExpPlusContext::PLUS() {
  return getToken(RQLParser::PLUS, 0);
}

RQLParser::ExpPlusContext::ExpPlusContext(Expression_factorContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpPlusContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpPlus(this);
}
void RQLParser::ExpPlusContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpPlus(this);
}
//----------------- ExpMinusContext ------------------------------------------------------------------

std::vector<RQLParser::Expression_factorContext *> RQLParser::ExpMinusContext::expression_factor() {
  return getRuleContexts<RQLParser::Expression_factorContext>();
}

RQLParser::Expression_factorContext* RQLParser::ExpMinusContext::expression_factor(size_t i) {
  return getRuleContext<RQLParser::Expression_factorContext>(i);
}

tree::TerminalNode* RQLParser::ExpMinusContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}

RQLParser::ExpMinusContext::ExpMinusContext(Expression_factorContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpMinusContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpMinus(this);
}
void RQLParser::ExpMinusContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpMinus(this);
}
//----------------- ExpTermContext ------------------------------------------------------------------

RQLParser::TermContext* RQLParser::ExpTermContext::term() {
  return getRuleContext<RQLParser::TermContext>(0);
}

RQLParser::ExpTermContext::ExpTermContext(Expression_factorContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpTermContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpTerm(this);
}
void RQLParser::ExpTermContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpTerm(this);
}

RQLParser::Expression_factorContext* RQLParser::expression_factor() {
   return expression_factor(0);
}

RQLParser::Expression_factorContext* RQLParser::expression_factor(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  RQLParser::Expression_factorContext *_localctx = _tracker.createInstance<Expression_factorContext>(_ctx, parentState);
  RQLParser::Expression_factorContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 44;
  enterRecursionRule(_localctx, 44, RQLParser::RuleExpression_factor, precedence);

    

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
    _localctx = _tracker.createInstance<ExpTermContext>(_localctx);
    _ctx = _localctx;
    previousContext = _localctx;

    setState(247);
    term(0);
    _ctx->stop = _input->LT(-1);
    setState(257);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 23, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(255);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 22, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpPlusContext>(_tracker.createInstance<Expression_factorContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_factor);
          setState(249);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(250);
          match(RQLParser::PLUS);
          setState(251);
          expression_factor(4);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpMinusContext>(_tracker.createInstance<Expression_factorContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_factor);
          setState(252);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(253);
          match(RQLParser::MINUS);
          setState(254);
          expression_factor(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(259);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 23, _ctx);
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


size_t RQLParser::TermContext::getRuleIndex() const {
  return RQLParser::RuleTerm;
}

void RQLParser::TermContext::copyFrom(TermContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ExpInContext ------------------------------------------------------------------

RQLParser::Expression_factorContext* RQLParser::ExpInContext::expression_factor() {
  return getRuleContext<RQLParser::Expression_factorContext>(0);
}

RQLParser::ExpInContext::ExpInContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpInContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpIn(this);
}
void RQLParser::ExpInContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpIn(this);
}
//----------------- ExpRationalContext ------------------------------------------------------------------

RQLParser::Fraction_ruleContext* RQLParser::ExpRationalContext::fraction_rule() {
  return getRuleContext<RQLParser::Fraction_ruleContext>(0);
}

RQLParser::ExpRationalContext::ExpRationalContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpRationalContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpRational(this);
}
void RQLParser::ExpRationalContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpRational(this);
}
//----------------- ExpFloatContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::ExpFloatContext::FLOAT() {
  return getToken(RQLParser::FLOAT, 0);
}

tree::TerminalNode* RQLParser::ExpFloatContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}

RQLParser::ExpFloatContext::ExpFloatContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpFloatContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpFloat(this);
}
void RQLParser::ExpFloatContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpFloat(this);
}
//----------------- ExpDecContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::ExpDecContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

tree::TerminalNode* RQLParser::ExpDecContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}

RQLParser::ExpDecContext::ExpDecContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpDecContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpDec(this);
}
void RQLParser::ExpDecContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpDec(this);
}
//----------------- ExpAggContext ------------------------------------------------------------------

RQLParser::AgregatorContext* RQLParser::ExpAggContext::agregator() {
  return getRuleContext<RQLParser::AgregatorContext>(0);
}

RQLParser::ExpAggContext::ExpAggContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpAggContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpAgg(this);
}
void RQLParser::ExpAggContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpAgg(this);
}
//----------------- ExpFnCallContext ------------------------------------------------------------------

RQLParser::Function_callContext* RQLParser::ExpFnCallContext::function_call() {
  return getRuleContext<RQLParser::Function_callContext>(0);
}

RQLParser::ExpFnCallContext::ExpFnCallContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpFnCallContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpFnCall(this);
}
void RQLParser::ExpFnCallContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpFnCall(this);
}
//----------------- ExpDivContext ------------------------------------------------------------------

std::vector<RQLParser::TermContext *> RQLParser::ExpDivContext::term() {
  return getRuleContexts<RQLParser::TermContext>();
}

RQLParser::TermContext* RQLParser::ExpDivContext::term(size_t i) {
  return getRuleContext<RQLParser::TermContext>(i);
}

tree::TerminalNode* RQLParser::ExpDivContext::DIVIDE() {
  return getToken(RQLParser::DIVIDE, 0);
}

RQLParser::ExpDivContext::ExpDivContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpDivContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpDiv(this);
}
void RQLParser::ExpDivContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpDiv(this);
}
//----------------- ExpFieldContext ------------------------------------------------------------------

RQLParser::Field_idContext* RQLParser::ExpFieldContext::field_id() {
  return getRuleContext<RQLParser::Field_idContext>(0);
}

RQLParser::ExpFieldContext::ExpFieldContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpFieldContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpField(this);
}
void RQLParser::ExpFieldContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpField(this);
}
//----------------- ExpNotContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::ExpNotContext::NOT_C() {
  return getToken(RQLParser::NOT_C, 0);
}

RQLParser::TermContext* RQLParser::ExpNotContext::term() {
  return getRuleContext<RQLParser::TermContext>(0);
}

RQLParser::ExpNotContext::ExpNotContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpNotContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpNot(this);
}
void RQLParser::ExpNotContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpNot(this);
}
//----------------- ExpStringContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::ExpStringContext::STRING() {
  return getToken(RQLParser::STRING, 0);
}

RQLParser::ExpStringContext::ExpStringContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpStringContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpString(this);
}
void RQLParser::ExpStringContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpString(this);
}
//----------------- ExpUnaryContext ------------------------------------------------------------------

RQLParser::Unary_op_expressionContext* RQLParser::ExpUnaryContext::unary_op_expression() {
  return getRuleContext<RQLParser::Unary_op_expressionContext>(0);
}

RQLParser::ExpUnaryContext::ExpUnaryContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpUnaryContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpUnary(this);
}
void RQLParser::ExpUnaryContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpUnary(this);
}
//----------------- ExpMultContext ------------------------------------------------------------------

std::vector<RQLParser::TermContext *> RQLParser::ExpMultContext::term() {
  return getRuleContexts<RQLParser::TermContext>();
}

RQLParser::TermContext* RQLParser::ExpMultContext::term(size_t i) {
  return getRuleContext<RQLParser::TermContext>(i);
}

tree::TerminalNode* RQLParser::ExpMultContext::STAR() {
  return getToken(RQLParser::STAR, 0);
}

RQLParser::ExpMultContext::ExpMultContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpMultContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpMult(this);
}
void RQLParser::ExpMultContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpMult(this);
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
  size_t startState = 46;
  enterRecursionRule(_localctx, 46, RQLParser::RuleTerm, precedence);

    size_t _la = 0;

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
    setState(281);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 26, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<ExpRationalContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(261);
      fraction_rule();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<ExpInContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(262);
      match(RQLParser::T__2);
      setState(263);
      expression_factor(0);
      setState(264);
      match(RQLParser::T__3);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<ExpFloatContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(267);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == RQLParser::MINUS) {
        setState(266);
        match(RQLParser::MINUS);
      }
      setState(269);
      match(RQLParser::FLOAT);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<ExpDecContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(271);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == RQLParser::MINUS) {
        setState(270);
        match(RQLParser::MINUS);
      }
      setState(273);
      match(RQLParser::DECIMAL);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<ExpStringContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(274);
      match(RQLParser::STRING);
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<ExpUnaryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(275);
      unary_op_expression();
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<ExpFieldContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(276);
      field_id();
      break;
    }

    case 8: {
      _localctx = _tracker.createInstance<ExpAggContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(277);
      agregator();
      break;
    }

    case 9: {
      _localctx = _tracker.createInstance<ExpFnCallContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(278);
      function_call();
      break;
    }

    case 10: {
      _localctx = _tracker.createInstance<ExpNotContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(279);
      match(RQLParser::NOT_C);
      setState(280);
      term(1);
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(291);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 28, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(289);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 27, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpMultContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm);
          setState(283);

          if (!(precpred(_ctx, 12))) throw FailedPredicateException(this, "precpred(_ctx, 12)");
          setState(284);
          match(RQLParser::STAR);
          setState(285);
          term(13);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpDivContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm);
          setState(286);

          if (!(precpred(_ctx, 11))) throw FailedPredicateException(this, "precpred(_ctx, 11)");
          setState(287);
          match(RQLParser::DIVIDE);
          setState(288);
          term(12);
          break;
        }

        default:
          break;
        } 
      }
      setState(293);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 28, _ctx);
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


size_t RQLParser::Stream_expressionContext::getRuleIndex() const {
  return RQLParser::RuleStream_expression;
}

void RQLParser::Stream_expressionContext::copyFrom(Stream_expressionContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- SExpPlusContext ------------------------------------------------------------------

std::vector<RQLParser::Stream_termContext *> RQLParser::SExpPlusContext::stream_term() {
  return getRuleContexts<RQLParser::Stream_termContext>();
}

RQLParser::Stream_termContext* RQLParser::SExpPlusContext::stream_term(size_t i) {
  return getRuleContext<RQLParser::Stream_termContext>(i);
}

tree::TerminalNode* RQLParser::SExpPlusContext::PLUS() {
  return getToken(RQLParser::PLUS, 0);
}

RQLParser::SExpPlusContext::SExpPlusContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpPlusContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpPlus(this);
}
void RQLParser::SExpPlusContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpPlus(this);
}
//----------------- SExpTermContext ------------------------------------------------------------------

RQLParser::Stream_termContext* RQLParser::SExpTermContext::stream_term() {
  return getRuleContext<RQLParser::Stream_termContext>(0);
}

RQLParser::SExpTermContext::SExpTermContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpTermContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpTerm(this);
}
void RQLParser::SExpTermContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpTerm(this);
}
//----------------- SExpTimeMoveContext ------------------------------------------------------------------

RQLParser::Stream_termContext* RQLParser::SExpTimeMoveContext::stream_term() {
  return getRuleContext<RQLParser::Stream_termContext>(0);
}

tree::TerminalNode* RQLParser::SExpTimeMoveContext::IS_GR() {
  return getToken(RQLParser::IS_GR, 0);
}

tree::TerminalNode* RQLParser::SExpTimeMoveContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

RQLParser::SExpTimeMoveContext::SExpTimeMoveContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpTimeMoveContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpTimeMove(this);
}
void RQLParser::SExpTimeMoveContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpTimeMove(this);
}
//----------------- SExpMinusContext ------------------------------------------------------------------

RQLParser::Stream_termContext* RQLParser::SExpMinusContext::stream_term() {
  return getRuleContext<RQLParser::Stream_termContext>(0);
}

tree::TerminalNode* RQLParser::SExpMinusContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}

RQLParser::Rational_seContext* RQLParser::SExpMinusContext::rational_se() {
  return getRuleContext<RQLParser::Rational_seContext>(0);
}

RQLParser::SExpMinusContext::SExpMinusContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpMinusContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpMinus(this);
}
void RQLParser::SExpMinusContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpMinus(this);
}
RQLParser::Stream_expressionContext* RQLParser::stream_expression() {
  Stream_expressionContext *_localctx = _tracker.createInstance<Stream_expressionContext>(_ctx, getState());
  enterRule(_localctx, 48, RQLParser::RuleStream_expression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(307);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::SExpTimeMoveContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(294);
      stream_term();
      setState(295);
      match(RQLParser::IS_GR);
      setState(296);
      match(RQLParser::DECIMAL);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::SExpMinusContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(298);
      stream_term();
      setState(299);
      match(RQLParser::MINUS);
      setState(300);
      rational_se();
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<RQLParser::SExpPlusContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(302);
      stream_term();
      setState(303);
      match(RQLParser::PLUS);
      setState(304);
      stream_term();
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<RQLParser::SExpTermContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(306);
      stream_term();
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

//----------------- Stream_termContext ------------------------------------------------------------------

RQLParser::Stream_termContext::Stream_termContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Stream_termContext::getRuleIndex() const {
  return RQLParser::RuleStream_term;
}

void RQLParser::Stream_termContext::copyFrom(Stream_termContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- SExpFactorContext ------------------------------------------------------------------

RQLParser::Stream_factorContext* RQLParser::SExpFactorContext::stream_factor() {
  return getRuleContext<RQLParser::Stream_factorContext>(0);
}

RQLParser::SExpFactorContext::SExpFactorContext(Stream_termContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpFactorContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpFactor(this);
}
void RQLParser::SExpFactorContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpFactor(this);
}
//----------------- SExpHashContext ------------------------------------------------------------------

std::vector<RQLParser::Stream_factorContext *> RQLParser::SExpHashContext::stream_factor() {
  return getRuleContexts<RQLParser::Stream_factorContext>();
}

RQLParser::Stream_factorContext* RQLParser::SExpHashContext::stream_factor(size_t i) {
  return getRuleContext<RQLParser::Stream_factorContext>(i);
}

tree::TerminalNode* RQLParser::SExpHashContext::SHARP() {
  return getToken(RQLParser::SHARP, 0);
}

RQLParser::SExpHashContext::SExpHashContext(Stream_termContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpHashContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpHash(this);
}
void RQLParser::SExpHashContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpHash(this);
}
//----------------- SExpModContext ------------------------------------------------------------------

RQLParser::Stream_factorContext* RQLParser::SExpModContext::stream_factor() {
  return getRuleContext<RQLParser::Stream_factorContext>(0);
}

tree::TerminalNode* RQLParser::SExpModContext::MOD() {
  return getToken(RQLParser::MOD, 0);
}

RQLParser::Rational_seContext* RQLParser::SExpModContext::rational_se() {
  return getRuleContext<RQLParser::Rational_seContext>(0);
}

RQLParser::SExpModContext::SExpModContext(Stream_termContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpModContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpMod(this);
}
void RQLParser::SExpModContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpMod(this);
}
//----------------- SExpAgregate_proformaContext ------------------------------------------------------------------

RQLParser::Stream_factorContext* RQLParser::SExpAgregate_proformaContext::stream_factor() {
  return getRuleContext<RQLParser::Stream_factorContext>(0);
}

tree::TerminalNode* RQLParser::SExpAgregate_proformaContext::DOT() {
  return getToken(RQLParser::DOT, 0);
}

RQLParser::AgregatorContext* RQLParser::SExpAgregate_proformaContext::agregator() {
  return getRuleContext<RQLParser::AgregatorContext>(0);
}

RQLParser::SExpAgregate_proformaContext::SExpAgregate_proformaContext(Stream_termContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpAgregate_proformaContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpAgregate_proforma(this);
}
void RQLParser::SExpAgregate_proformaContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpAgregate_proforma(this);
}
//----------------- SExpAgseContext ------------------------------------------------------------------

RQLParser::Stream_factorContext* RQLParser::SExpAgseContext::stream_factor() {
  return getRuleContext<RQLParser::Stream_factorContext>(0);
}

tree::TerminalNode* RQLParser::SExpAgseContext::AT() {
  return getToken(RQLParser::AT, 0);
}

tree::TerminalNode* RQLParser::SExpAgseContext::COMMA() {
  return getToken(RQLParser::COMMA, 0);
}

std::vector<tree::TerminalNode *> RQLParser::SExpAgseContext::DECIMAL() {
  return getTokens(RQLParser::DECIMAL);
}

tree::TerminalNode* RQLParser::SExpAgseContext::DECIMAL(size_t i) {
  return getToken(RQLParser::DECIMAL, i);
}

tree::TerminalNode* RQLParser::SExpAgseContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}

RQLParser::SExpAgseContext::SExpAgseContext(Stream_termContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpAgseContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpAgse(this);
}
void RQLParser::SExpAgseContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpAgse(this);
}
//----------------- SExpAndContext ------------------------------------------------------------------

RQLParser::Stream_factorContext* RQLParser::SExpAndContext::stream_factor() {
  return getRuleContext<RQLParser::Stream_factorContext>(0);
}

tree::TerminalNode* RQLParser::SExpAndContext::AND() {
  return getToken(RQLParser::AND, 0);
}

RQLParser::Rational_seContext* RQLParser::SExpAndContext::rational_se() {
  return getRuleContext<RQLParser::Rational_seContext>(0);
}

RQLParser::SExpAndContext::SExpAndContext(Stream_termContext *ctx) { copyFrom(ctx); }

void RQLParser::SExpAndContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSExpAnd(this);
}
void RQLParser::SExpAndContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSExpAnd(this);
}
RQLParser::Stream_termContext* RQLParser::stream_term() {
  Stream_termContext *_localctx = _tracker.createInstance<Stream_termContext>(_ctx, getState());
  enterRule(_localctx, 50, RQLParser::RuleStream_term);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(337);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 31, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::SExpHashContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(309);
      stream_factor();
      setState(310);
      match(RQLParser::SHARP);
      setState(311);
      stream_factor();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::SExpAndContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(313);
      stream_factor();
      setState(314);
      match(RQLParser::AND);
      setState(315);
      rational_se();
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<RQLParser::SExpModContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(317);
      stream_factor();
      setState(318);
      match(RQLParser::MOD);
      setState(319);
      rational_se();
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<RQLParser::SExpAgseContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(321);
      stream_factor();
      setState(322);
      match(RQLParser::AT);
      setState(323);
      match(RQLParser::T__2);
      setState(324);
      antlrcpp::downCast<SExpAgseContext *>(_localctx)->step = match(RQLParser::DECIMAL);
      setState(325);
      match(RQLParser::COMMA);
      setState(327);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == RQLParser::MINUS) {
        setState(326);
        match(RQLParser::MINUS);
      }
      setState(329);
      antlrcpp::downCast<SExpAgseContext *>(_localctx)->window = match(RQLParser::DECIMAL);
      setState(330);
      match(RQLParser::T__3);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<RQLParser::SExpAgregate_proformaContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(332);
      stream_factor();
      setState(333);
      match(RQLParser::DOT);
      setState(334);
      agregator();
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<RQLParser::SExpFactorContext>(_localctx);
      enterOuterAlt(_localctx, 6);
      setState(336);
      stream_factor();
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
  enterRule(_localctx, 52, RQLParser::RuleStream_factor);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(344);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::ID: {
        enterOuterAlt(_localctx, 1);
        setState(339);
        match(RQLParser::ID);
        break;
      }

      case RQLParser::T__2: {
        enterOuterAlt(_localctx, 2);
        setState(340);
        match(RQLParser::T__2);
        setState(341);
        stream_expression();
        setState(342);
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


size_t RQLParser::AgregatorContext::getRuleIndex() const {
  return RQLParser::RuleAgregator;
}

void RQLParser::AgregatorContext::copyFrom(AgregatorContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- StreamMinContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::StreamMinContext::MIN() {
  return getToken(RQLParser::MIN, 0);
}

RQLParser::StreamMinContext::StreamMinContext(AgregatorContext *ctx) { copyFrom(ctx); }

void RQLParser::StreamMinContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStreamMin(this);
}
void RQLParser::StreamMinContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStreamMin(this);
}
//----------------- StreamAvgContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::StreamAvgContext::AVG() {
  return getToken(RQLParser::AVG, 0);
}

RQLParser::StreamAvgContext::StreamAvgContext(AgregatorContext *ctx) { copyFrom(ctx); }

void RQLParser::StreamAvgContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStreamAvg(this);
}
void RQLParser::StreamAvgContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStreamAvg(this);
}
//----------------- StreamMaxContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::StreamMaxContext::MAX() {
  return getToken(RQLParser::MAX, 0);
}

RQLParser::StreamMaxContext::StreamMaxContext(AgregatorContext *ctx) { copyFrom(ctx); }

void RQLParser::StreamMaxContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStreamMax(this);
}
void RQLParser::StreamMaxContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStreamMax(this);
}
//----------------- StreamSumContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::StreamSumContext::SUMC() {
  return getToken(RQLParser::SUMC, 0);
}

RQLParser::StreamSumContext::StreamSumContext(AgregatorContext *ctx) { copyFrom(ctx); }

void RQLParser::StreamSumContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStreamSum(this);
}
void RQLParser::StreamSumContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStreamSum(this);
}
RQLParser::AgregatorContext* RQLParser::agregator() {
  AgregatorContext *_localctx = _tracker.createInstance<AgregatorContext>(_ctx, getState());
  enterRule(_localctx, 54, RQLParser::RuleAgregator);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(350);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::MIN: {
        _localctx = _tracker.createInstance<RQLParser::StreamMinContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(346);
        match(RQLParser::MIN);
        break;
      }

      case RQLParser::MAX: {
        _localctx = _tracker.createInstance<RQLParser::StreamMaxContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(347);
        match(RQLParser::MAX);
        break;
      }

      case RQLParser::AVG: {
        _localctx = _tracker.createInstance<RQLParser::StreamAvgContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(348);
        match(RQLParser::AVG);
        break;
      }

      case RQLParser::SUMC: {
        _localctx = _tracker.createInstance<RQLParser::StreamSumContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(349);
        match(RQLParser::SUMC);
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

//----------------- Function_callContext ------------------------------------------------------------------

RQLParser::Function_callContext::Function_callContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<RQLParser::Expression_factorContext *> RQLParser::Function_callContext::expression_factor() {
  return getRuleContexts<RQLParser::Expression_factorContext>();
}

RQLParser::Expression_factorContext* RQLParser::Function_callContext::expression_factor(size_t i) {
  return getRuleContext<RQLParser::Expression_factorContext>(i);
}

std::vector<tree::TerminalNode *> RQLParser::Function_callContext::COMMA() {
  return getTokens(RQLParser::COMMA);
}

tree::TerminalNode* RQLParser::Function_callContext::COMMA(size_t i) {
  return getToken(RQLParser::COMMA, i);
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
  enterRule(_localctx, 56, RQLParser::RuleFunction_call);
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
    setState(352);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 2097120) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(353);
    match(RQLParser::T__2);
    setState(354);
    expression_factor(0);
    setState(359);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(355);
      match(RQLParser::COMMA);
      setState(356);
      expression_factor(0);
      setState(361);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(362);
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
    case 20: return expression_logicSempred(antlrcpp::downCast<Expression_logicContext *>(context), predicateIndex);
    case 21: return term_logicSempred(antlrcpp::downCast<Term_logicContext *>(context), predicateIndex);
    case 22: return expression_factorSempred(antlrcpp::downCast<Expression_factorContext *>(context), predicateIndex);
    case 23: return termSempred(antlrcpp::downCast<TermContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool RQLParser::expression_logicSempred(Expression_logicContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 3);
    case 1: return precpred(_ctx, 2);

  default:
    break;
  }
  return true;
}

bool RQLParser::term_logicSempred(Term_logicContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 2: return precpred(_ctx, 7);
    case 3: return precpred(_ctx, 6);
    case 4: return precpred(_ctx, 5);
    case 5: return precpred(_ctx, 4);
    case 6: return precpred(_ctx, 3);
    case 7: return precpred(_ctx, 2);

  default:
    break;
  }
  return true;
}

bool RQLParser::expression_factorSempred(Expression_factorContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 8: return precpred(_ctx, 3);
    case 9: return precpred(_ctx, 2);

  default:
    break;
  }
  return true;
}

bool RQLParser::termSempred(TermContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 10: return precpred(_ctx, 12);
    case 11: return precpred(_ctx, 11);

  default:
    break;
  }
  return true;
}

void RQLParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  rqlParserInitialize();
#else
  ::antlr4::internal::call_once(rqlParserOnceFlag, rqlParserInitialize);
#endif
}
