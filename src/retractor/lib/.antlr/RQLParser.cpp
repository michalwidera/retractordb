
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
      "prog", "compiler_option", "select_statement", "declare_statement", 
      "rule_statement", "dumppart", "systempart", "rational_se", "retention_from", 
      "fraction_rule", "field_declaration", "field_type", "select_list", 
      "field_id", "unary_op_expression", "asterisk", "expression", "logic", 
      "expression_logic", "term_logic", "expression_factor", "term", "stream_expression", 
      "stream_factor", "gen_index", "agregator", "stream_fn_call", "function_call"
    },
    std::vector<std::string>{
      "", "'['", "']'", "'('", "')'", "'Sqrt'", "'Ceil'", "'Abs'", "'Floor'", 
      "'Sign'", "'Chr'", "'Length'", "'ToNumber'", "'ToTimeStamp'", "'FloatCast'", 
      "'IntCast'", "'Count'", "'Crc'", "'Sum'", "'IsZero'", "'IsNonZero'", 
      "'isnull'", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "'to_integer'", "'to_float'", "'to_double'", "'to_string'", 
      "", "", "", "", "", "'='", "'!='", "'>'", "'<'", "'>='", "'<='", "'!'", 
      "'||'", "'.'", "'_'", "'@'", "'#'", "'&'", "'%'", "'$'", "','", "';'", 
      "':'", "'::'", "'*'", "'/'", "'+'", "'-'", "'~'", "'|'", "'^'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "BYTE_T", "STRING_T", "UNSIGNED_T", "INTEGER_T", 
      "FLOAT_T", "DOUBLE_T", "SELECT", "STREAM", "FROM", "DECLARE", "RETENTION", 
      "FILE", "STORAGE", "ROTATION", "SUBSTRAT", "RULE", "DISPOSABLE", "ONESHOT", 
      "HOLD", "VOLATILE", "ON", "WHEN", "DUMP", "SYSTEM", "DO", "TO", "AND_C", 
      "OR_C", "NOT_C", "MIN", "MAX", "AVG", "SUMC", "TYPE_PROFILE", "STRING_PROFILE", 
      "TO_INTEGER_FN", "TO_FLOAT_FN", "TO_DOUBLE_FN", "TO_STRING_FN", "ID", 
      "STRING", "FLOAT", "DECIMAL", "REAL", "IS_EQ", "IS_NQ", "IS_GR", "IS_LS", 
      "IS_GE", "IS_LE", "EXCLAMATION", "DOUBLE_BAR", "DOT", "UNDERLINE", 
      "AT", "SHARP", "AND", "MOD", "DOLLAR", "COMMA", "SEMI", "COLON", "DOUBLE_COLON", 
      "STAR", "DIVIDE", "PLUS", "MINUS", "BIT_NOT", "BIT_OR", "BIT_XOR", 
      "SPACE", "COMMENT", "LINE_COMMENT2"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,94,416,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,1,0,1,
  	0,1,0,1,0,4,0,61,8,0,11,0,12,0,62,1,0,1,0,1,1,1,1,1,1,1,2,1,2,1,2,1,2,
  	1,2,1,2,1,2,3,2,77,8,2,1,2,1,2,1,2,1,2,3,2,83,8,2,1,2,3,2,86,8,2,1,2,
  	3,2,89,8,2,1,2,1,2,3,2,93,8,2,1,3,1,3,1,3,1,3,5,3,99,8,3,10,3,12,3,102,
  	9,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,3,3,111,8,3,1,3,3,3,114,8,3,1,3,3,3,117,
  	8,3,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,3,4,128,8,4,1,5,1,5,3,5,132,8,
  	5,1,5,1,5,1,5,3,5,137,8,5,1,5,1,5,1,5,3,5,142,8,5,1,6,1,6,1,6,1,7,1,7,
  	1,7,3,7,150,8,7,1,8,1,8,1,8,3,8,155,8,8,1,9,1,9,1,9,1,9,1,10,1,10,1,10,
  	1,10,1,10,3,10,166,8,10,1,11,1,11,1,11,1,11,1,11,1,11,3,11,174,8,11,1,
  	12,1,12,1,12,1,12,5,12,180,8,12,10,12,12,12,183,9,12,3,12,185,8,12,1,
  	13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,
  	13,1,13,1,13,3,13,204,8,13,1,14,1,14,1,14,1,14,3,14,210,8,14,1,15,1,15,
  	3,15,214,8,15,1,15,1,15,1,16,1,16,1,17,1,17,1,18,1,18,1,18,1,18,1,18,
  	1,18,1,18,1,18,1,18,5,18,231,8,18,10,18,12,18,234,9,18,1,19,1,19,1,19,
  	1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,
  	1,19,1,19,1,19,1,19,5,19,257,8,19,10,19,12,19,260,9,19,1,20,1,20,1,20,
  	1,20,1,20,1,20,1,20,1,20,1,20,5,20,271,8,20,10,20,12,20,274,9,20,1,21,
  	1,21,1,21,1,21,1,21,1,21,3,21,282,8,21,1,21,1,21,3,21,286,8,21,1,21,1,
  	21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,3,21,297,8,21,1,21,1,21,1,21,1,
  	21,1,21,1,21,5,21,305,8,21,10,21,12,21,308,9,21,1,22,1,22,1,22,1,22,1,
  	22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,3,22,325,8,22,1,
  	22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,1,22,1,
  	22,1,22,1,22,5,22,344,8,22,10,22,12,22,347,9,22,1,23,1,23,1,23,1,23,1,
  	23,1,23,1,23,1,23,1,23,1,23,1,23,3,23,360,8,23,1,24,1,24,1,24,1,24,1,
  	24,1,24,1,24,3,24,369,8,24,1,24,1,24,1,24,1,24,1,24,1,24,5,24,377,8,24,
  	10,24,12,24,380,9,24,1,25,1,25,1,25,1,25,3,25,386,8,25,1,26,1,26,1,26,
  	1,26,1,26,1,27,1,27,1,27,1,27,1,27,5,27,398,8,27,10,27,12,27,401,9,27,
  	1,27,1,27,1,27,1,27,1,27,1,27,1,27,3,27,410,8,27,1,27,1,27,3,27,414,8,
  	27,1,27,0,6,36,38,40,42,44,48,28,0,2,4,6,8,10,12,14,16,18,20,22,24,26,
  	28,30,32,34,36,38,40,42,44,46,48,50,52,54,0,5,1,0,34,36,2,0,56,56,62,
  	62,1,0,87,88,1,0,51,54,2,0,5,21,57,59,466,0,60,1,0,0,0,2,66,1,0,0,0,4,
  	69,1,0,0,0,6,94,1,0,0,0,8,118,1,0,0,0,10,129,1,0,0,0,12,143,1,0,0,0,14,
  	149,1,0,0,0,16,151,1,0,0,0,18,156,1,0,0,0,20,160,1,0,0,0,22,173,1,0,0,
  	0,24,184,1,0,0,0,26,203,1,0,0,0,28,209,1,0,0,0,30,213,1,0,0,0,32,217,
  	1,0,0,0,34,219,1,0,0,0,36,221,1,0,0,0,38,235,1,0,0,0,40,261,1,0,0,0,42,
  	296,1,0,0,0,44,309,1,0,0,0,46,359,1,0,0,0,48,368,1,0,0,0,50,385,1,0,0,
  	0,52,387,1,0,0,0,54,413,1,0,0,0,56,61,3,4,2,0,57,61,3,6,3,0,58,61,3,2,
  	1,0,59,61,3,8,4,0,60,56,1,0,0,0,60,57,1,0,0,0,60,58,1,0,0,0,60,59,1,0,
  	0,0,61,62,1,0,0,0,62,60,1,0,0,0,62,63,1,0,0,0,63,64,1,0,0,0,64,65,5,0,
  	0,1,65,1,1,0,0,0,66,67,7,0,0,0,67,68,7,1,0,0,68,3,1,0,0,0,69,70,5,28,
  	0,0,70,71,3,24,12,0,71,72,5,29,0,0,72,76,5,61,0,0,73,74,5,1,0,0,74,75,
  	5,64,0,0,75,77,5,2,0,0,76,73,1,0,0,0,76,77,1,0,0,0,77,78,1,0,0,0,78,79,
  	5,30,0,0,79,82,3,44,22,0,80,81,5,33,0,0,81,83,5,62,0,0,82,80,1,0,0,0,
  	82,83,1,0,0,0,83,85,1,0,0,0,84,86,3,16,8,0,85,84,1,0,0,0,85,86,1,0,0,
  	0,86,88,1,0,0,0,87,89,5,41,0,0,88,87,1,0,0,0,88,89,1,0,0,0,89,92,1,0,
  	0,0,90,91,5,34,0,0,91,93,5,55,0,0,92,90,1,0,0,0,92,93,1,0,0,0,93,5,1,
  	0,0,0,94,95,5,31,0,0,95,100,3,20,10,0,96,97,5,81,0,0,97,99,3,20,10,0,
  	98,96,1,0,0,0,99,102,1,0,0,0,100,98,1,0,0,0,100,101,1,0,0,0,101,103,1,
  	0,0,0,102,100,1,0,0,0,103,104,5,29,0,0,104,105,5,61,0,0,105,106,5,81,
  	0,0,106,107,3,14,7,0,107,108,5,33,0,0,108,110,5,62,0,0,109,111,5,38,0,
  	0,110,109,1,0,0,0,110,111,1,0,0,0,111,113,1,0,0,0,112,114,5,39,0,0,113,
  	112,1,0,0,0,113,114,1,0,0,0,114,116,1,0,0,0,115,117,5,40,0,0,116,115,
  	1,0,0,0,116,117,1,0,0,0,117,7,1,0,0,0,118,119,5,37,0,0,119,120,5,61,0,
  	0,120,121,5,42,0,0,121,122,5,61,0,0,122,123,5,43,0,0,123,124,3,34,17,
  	0,124,127,5,46,0,0,125,128,3,10,5,0,126,128,3,12,6,0,127,125,1,0,0,0,
  	127,126,1,0,0,0,128,9,1,0,0,0,129,131,5,44,0,0,130,132,5,88,0,0,131,130,
  	1,0,0,0,131,132,1,0,0,0,132,133,1,0,0,0,133,134,5,64,0,0,134,136,5,47,
  	0,0,135,137,5,88,0,0,136,135,1,0,0,0,136,137,1,0,0,0,137,138,1,0,0,0,
  	138,141,5,64,0,0,139,140,5,32,0,0,140,142,5,64,0,0,141,139,1,0,0,0,141,
  	142,1,0,0,0,142,11,1,0,0,0,143,144,5,45,0,0,144,145,5,62,0,0,145,13,1,
  	0,0,0,146,150,3,18,9,0,147,150,5,63,0,0,148,150,5,64,0,0,149,146,1,0,
  	0,0,149,147,1,0,0,0,149,148,1,0,0,0,150,15,1,0,0,0,151,152,5,32,0,0,152,
  	154,5,64,0,0,153,155,5,64,0,0,154,153,1,0,0,0,154,155,1,0,0,0,155,17,
  	1,0,0,0,156,157,5,64,0,0,157,158,5,86,0,0,158,159,5,64,0,0,159,19,1,0,
  	0,0,160,161,5,61,0,0,161,165,3,22,11,0,162,163,5,1,0,0,163,164,5,64,0,
  	0,164,166,5,2,0,0,165,162,1,0,0,0,165,166,1,0,0,0,166,21,1,0,0,0,167,
  	174,5,22,0,0,168,174,5,25,0,0,169,174,5,24,0,0,170,174,5,26,0,0,171,174,
  	5,27,0,0,172,174,5,23,0,0,173,167,1,0,0,0,173,168,1,0,0,0,173,169,1,0,
  	0,0,173,170,1,0,0,0,173,171,1,0,0,0,173,172,1,0,0,0,174,23,1,0,0,0,175,
  	185,3,30,15,0,176,181,3,32,16,0,177,178,5,81,0,0,178,180,3,32,16,0,179,
  	177,1,0,0,0,180,183,1,0,0,0,181,179,1,0,0,0,181,182,1,0,0,0,182,185,1,
  	0,0,0,183,181,1,0,0,0,184,175,1,0,0,0,184,176,1,0,0,0,185,25,1,0,0,0,
  	186,204,5,61,0,0,187,188,5,61,0,0,188,189,5,1,0,0,189,190,5,75,0,0,190,
  	204,5,2,0,0,191,192,5,61,0,0,192,193,5,74,0,0,193,204,5,61,0,0,194,195,
  	5,61,0,0,195,196,5,1,0,0,196,197,5,64,0,0,197,204,5,2,0,0,198,199,5,61,
  	0,0,199,200,5,1,0,0,200,201,3,48,24,0,201,202,5,2,0,0,202,204,1,0,0,0,
  	203,186,1,0,0,0,203,187,1,0,0,0,203,191,1,0,0,0,203,194,1,0,0,0,203,198,
  	1,0,0,0,204,27,1,0,0,0,205,206,5,89,0,0,206,210,3,32,16,0,207,208,7,2,
  	0,0,208,210,3,32,16,0,209,205,1,0,0,0,209,207,1,0,0,0,210,29,1,0,0,0,
  	211,212,5,61,0,0,212,214,5,74,0,0,213,211,1,0,0,0,213,214,1,0,0,0,214,
  	215,1,0,0,0,215,216,5,85,0,0,216,31,1,0,0,0,217,218,3,40,20,0,218,33,
  	1,0,0,0,219,220,3,36,18,0,220,35,1,0,0,0,221,222,6,18,-1,0,222,223,3,
  	38,19,0,223,232,1,0,0,0,224,225,10,3,0,0,225,226,5,48,0,0,226,231,3,36,
  	18,4,227,228,10,2,0,0,228,229,5,49,0,0,229,231,3,36,18,3,230,224,1,0,
  	0,0,230,227,1,0,0,0,231,234,1,0,0,0,232,230,1,0,0,0,232,233,1,0,0,0,233,
  	37,1,0,0,0,234,232,1,0,0,0,235,236,6,19,-1,0,236,237,3,40,20,0,237,258,
  	1,0,0,0,238,239,10,7,0,0,239,240,5,66,0,0,240,257,3,38,19,8,241,242,10,
  	6,0,0,242,243,5,67,0,0,243,257,3,38,19,7,244,245,10,5,0,0,245,246,5,68,
  	0,0,246,257,3,38,19,6,247,248,10,4,0,0,248,249,5,69,0,0,249,257,3,38,
  	19,5,250,251,10,3,0,0,251,252,5,70,0,0,252,257,3,38,19,4,253,254,10,2,
  	0,0,254,255,5,71,0,0,255,257,3,38,19,3,256,238,1,0,0,0,256,241,1,0,0,
  	0,256,244,1,0,0,0,256,247,1,0,0,0,256,250,1,0,0,0,256,253,1,0,0,0,257,
  	260,1,0,0,0,258,256,1,0,0,0,258,259,1,0,0,0,259,39,1,0,0,0,260,258,1,
  	0,0,0,261,262,6,20,-1,0,262,263,3,42,21,0,263,272,1,0,0,0,264,265,10,
  	3,0,0,265,266,5,87,0,0,266,271,3,40,20,4,267,268,10,2,0,0,268,269,5,88,
  	0,0,269,271,3,40,20,3,270,264,1,0,0,0,270,267,1,0,0,0,271,274,1,0,0,0,
  	272,270,1,0,0,0,272,273,1,0,0,0,273,41,1,0,0,0,274,272,1,0,0,0,275,276,
  	6,21,-1,0,276,277,5,3,0,0,277,278,3,40,20,0,278,279,5,4,0,0,279,297,1,
  	0,0,0,280,282,5,88,0,0,281,280,1,0,0,0,281,282,1,0,0,0,282,283,1,0,0,
  	0,283,297,5,63,0,0,284,286,5,88,0,0,285,284,1,0,0,0,285,286,1,0,0,0,286,
  	287,1,0,0,0,287,297,5,64,0,0,288,297,5,62,0,0,289,297,3,28,14,0,290,297,
  	3,26,13,0,291,297,3,50,25,0,292,297,5,80,0,0,293,297,3,54,27,0,294,295,
  	5,50,0,0,295,297,3,42,21,1,296,275,1,0,0,0,296,281,1,0,0,0,296,285,1,
  	0,0,0,296,288,1,0,0,0,296,289,1,0,0,0,296,290,1,0,0,0,296,291,1,0,0,0,
  	296,292,1,0,0,0,296,293,1,0,0,0,296,294,1,0,0,0,297,306,1,0,0,0,298,299,
  	10,12,0,0,299,300,5,85,0,0,300,305,3,42,21,13,301,302,10,11,0,0,302,303,
  	5,86,0,0,303,305,3,42,21,12,304,298,1,0,0,0,304,301,1,0,0,0,305,308,1,
  	0,0,0,306,304,1,0,0,0,306,307,1,0,0,0,307,43,1,0,0,0,308,306,1,0,0,0,
  	309,310,6,22,-1,0,310,311,3,46,23,0,311,345,1,0,0,0,312,313,10,3,0,0,
  	313,314,5,77,0,0,314,344,3,44,22,4,315,316,10,2,0,0,316,317,5,87,0,0,
  	317,344,3,44,22,3,318,319,10,9,0,0,319,320,5,76,0,0,320,321,5,3,0,0,321,
  	322,5,64,0,0,322,324,5,81,0,0,323,325,5,88,0,0,324,323,1,0,0,0,324,325,
  	1,0,0,0,325,326,1,0,0,0,326,327,5,64,0,0,327,344,5,4,0,0,328,329,10,8,
  	0,0,329,330,5,78,0,0,330,344,3,14,7,0,331,332,10,7,0,0,332,333,5,79,0,
  	0,333,344,3,14,7,0,334,335,10,6,0,0,335,336,5,68,0,0,336,344,5,64,0,0,
  	337,338,10,5,0,0,338,339,5,88,0,0,339,344,3,14,7,0,340,341,10,4,0,0,341,
  	342,5,74,0,0,342,344,3,50,25,0,343,312,1,0,0,0,343,315,1,0,0,0,343,318,
  	1,0,0,0,343,328,1,0,0,0,343,331,1,0,0,0,343,334,1,0,0,0,343,337,1,0,0,
  	0,343,340,1,0,0,0,344,347,1,0,0,0,345,343,1,0,0,0,345,346,1,0,0,0,346,
  	45,1,0,0,0,347,345,1,0,0,0,348,360,5,61,0,0,349,350,5,61,0,0,350,351,
  	5,1,0,0,351,352,3,48,24,0,352,353,5,2,0,0,353,360,1,0,0,0,354,355,5,3,
  	0,0,355,356,3,44,22,0,356,357,5,4,0,0,357,360,1,0,0,0,358,360,3,52,26,
  	0,359,348,1,0,0,0,359,349,1,0,0,0,359,354,1,0,0,0,359,358,1,0,0,0,360,
  	47,1,0,0,0,361,362,6,24,-1,0,362,363,5,3,0,0,363,364,3,48,24,0,364,365,
  	5,4,0,0,365,369,1,0,0,0,366,369,5,80,0,0,367,369,5,64,0,0,368,361,1,0,
  	0,0,368,366,1,0,0,0,368,367,1,0,0,0,369,378,1,0,0,0,370,371,10,5,0,0,
  	371,372,5,85,0,0,372,377,3,48,24,6,373,374,10,4,0,0,374,375,7,2,0,0,375,
  	377,3,48,24,5,376,370,1,0,0,0,376,373,1,0,0,0,377,380,1,0,0,0,378,376,
  	1,0,0,0,378,379,1,0,0,0,379,49,1,0,0,0,380,378,1,0,0,0,381,386,5,51,0,
  	0,382,386,5,52,0,0,383,386,5,53,0,0,384,386,5,54,0,0,385,381,1,0,0,0,
  	385,382,1,0,0,0,385,383,1,0,0,0,385,384,1,0,0,0,386,51,1,0,0,0,387,388,
  	7,3,0,0,388,389,5,3,0,0,389,390,3,44,22,0,390,391,5,4,0,0,391,53,1,0,
  	0,0,392,393,7,4,0,0,393,394,5,3,0,0,394,399,3,40,20,0,395,396,5,81,0,
  	0,396,398,3,40,20,0,397,395,1,0,0,0,398,401,1,0,0,0,399,397,1,0,0,0,399,
  	400,1,0,0,0,400,402,1,0,0,0,401,399,1,0,0,0,402,403,5,4,0,0,403,414,1,
  	0,0,0,404,405,5,60,0,0,405,406,5,3,0,0,406,409,3,40,20,0,407,408,5,83,
  	0,0,408,410,5,64,0,0,409,407,1,0,0,0,409,410,1,0,0,0,410,411,1,0,0,0,
  	411,412,5,4,0,0,412,414,1,0,0,0,413,392,1,0,0,0,413,404,1,0,0,0,414,55,
  	1,0,0,0,46,60,62,76,82,85,88,92,100,110,113,116,127,131,136,141,149,154,
  	165,173,181,184,203,209,213,230,232,256,258,270,272,281,285,296,304,306,
  	324,343,345,359,368,376,378,385,399,409,413
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
    setState(60); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(60);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::SELECT: {
          setState(56);
          select_statement();
          break;
        }

        case RQLParser::DECLARE: {
          setState(57);
          declare_statement();
          break;
        }

        case RQLParser::STORAGE:
        case RQLParser::ROTATION:
        case RQLParser::SUBSTRAT: {
          setState(58);
          compiler_option();
          break;
        }

        case RQLParser::RULE: {
          setState(59);
          rule_statement();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(62); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 260113956864) != 0));
    setState(64);
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

tree::TerminalNode* RQLParser::CoptionContext::ROTATION() {
  return getToken(RQLParser::ROTATION, 0);
}

tree::TerminalNode* RQLParser::CoptionContext::STORAGE() {
  return getToken(RQLParser::STORAGE, 0);
}

tree::TerminalNode* RQLParser::CoptionContext::SUBSTRAT() {
  return getToken(RQLParser::SUBSTRAT, 0);
}

tree::TerminalNode* RQLParser::CoptionContext::STRING_PROFILE() {
  return getToken(RQLParser::STRING_PROFILE, 0);
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
  size_t _la = 0;

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
    setState(66);
    antlrcpp::downCast<CoptionContext *>(_localctx)->directive = _input->LT(1);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 120259084288) != 0))) {
      antlrcpp::downCast<CoptionContext *>(_localctx)->directive = _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(67);
    antlrcpp::downCast<CoptionContext *>(_localctx)->value = _input->LT(1);
    _la = _input->LA(1);
    if (!(_la == RQLParser::STRING_PROFILE

    || _la == RQLParser::STRING)) {
      antlrcpp::downCast<CoptionContext *>(_localctx)->value = _errHandler->recoverInline(this);
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

tree::TerminalNode* RQLParser::SelectContext::VOLATILE() {
  return getToken(RQLParser::VOLATILE, 0);
}

tree::TerminalNode* RQLParser::SelectContext::STORAGE() {
  return getToken(RQLParser::STORAGE, 0);
}

tree::TerminalNode* RQLParser::SelectContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

tree::TerminalNode* RQLParser::SelectContext::STRING() {
  return getToken(RQLParser::STRING, 0);
}

tree::TerminalNode* RQLParser::SelectContext::TYPE_PROFILE() {
  return getToken(RQLParser::TYPE_PROFILE, 0);
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
  enterRule(_localctx, 4, RQLParser::RuleSelect_statement);
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
    setState(69);
    match(RQLParser::SELECT);
    setState(70);
    select_list();
    setState(71);
    match(RQLParser::STREAM);
    setState(72);
    antlrcpp::downCast<SelectContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(76);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::T__0) {
      setState(73);
      match(RQLParser::T__0);
      setState(74);
      antlrcpp::downCast<SelectContext *>(_localctx)->gen_size = match(RQLParser::DECIMAL);
      setState(75);
      match(RQLParser::T__1);
    }
    setState(78);
    match(RQLParser::FROM);
    setState(79);
    stream_expression(0);
    setState(82);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::FILE) {
      setState(80);
      match(RQLParser::FILE);
      setState(81);
      antlrcpp::downCast<SelectContext *>(_localctx)->file_name = match(RQLParser::STRING);
    }
    setState(85);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::RETENTION) {
      setState(84);
      retention_from();
    }
    setState(88);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::VOLATILE) {
      setState(87);
      match(RQLParser::VOLATILE);
    }
    setState(92);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 6, _ctx)) {
    case 1: {
      setState(90);
      match(RQLParser::STORAGE);
      setState(91);
      antlrcpp::downCast<SelectContext *>(_localctx)->type_name = match(RQLParser::TYPE_PROFILE);
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

tree::TerminalNode* RQLParser::DeclareContext::DISPOSABLE() {
  return getToken(RQLParser::DISPOSABLE, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::ONESHOT() {
  return getToken(RQLParser::ONESHOT, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::HOLD() {
  return getToken(RQLParser::HOLD, 0);
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
    setState(94);
    match(RQLParser::DECLARE);
    setState(95);
    field_declaration();
    setState(100);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(96);
      match(RQLParser::COMMA);
      setState(97);
      field_declaration();
      setState(102);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(103);
    match(RQLParser::STREAM);
    setState(104);
    antlrcpp::downCast<DeclareContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(105);
    match(RQLParser::COMMA);
    setState(106);
    rational_se();
    setState(107);
    match(RQLParser::FILE);
    setState(108);
    antlrcpp::downCast<DeclareContext *>(_localctx)->file_name = match(RQLParser::STRING);
    setState(110);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::DISPOSABLE) {
      setState(109);
      match(RQLParser::DISPOSABLE);
    }
    setState(113);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::ONESHOT) {
      setState(112);
      match(RQLParser::ONESHOT);
    }
    setState(116);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::HOLD) {
      setState(115);
      match(RQLParser::HOLD);
    }
   
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
  enterRule(_localctx, 8, RQLParser::RuleRule_statement);

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
    setState(118);
    match(RQLParser::RULE);
    setState(119);
    antlrcpp::downCast<RulezContext *>(_localctx)->name = match(RQLParser::ID);
    setState(120);
    match(RQLParser::ON);
    setState(121);
    antlrcpp::downCast<RulezContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(122);
    match(RQLParser::WHEN);
    setState(123);
    logic();
    setState(124);
    match(RQLParser::DO);
    setState(127);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::DUMP: {
        setState(125);
        dumppart();
        break;
      }

      case RQLParser::SYSTEM: {
        setState(126);
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
  enterRule(_localctx, 10, RQLParser::RuleDumppart);
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
    setState(129);
    match(RQLParser::DUMP);
    setState(131);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::MINUS) {
      setState(130);
      match(RQLParser::MINUS);
    }
    setState(133);
    antlrcpp::downCast<DumppartContext *>(_localctx)->step_back = match(RQLParser::DECIMAL);
    setState(134);
    match(RQLParser::TO);
    setState(136);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::MINUS) {
      setState(135);
      match(RQLParser::MINUS);
    }
    setState(138);
    antlrcpp::downCast<DumppartContext *>(_localctx)->step_forward = match(RQLParser::DECIMAL);
    setState(141);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::RETENTION) {
      setState(139);
      match(RQLParser::RETENTION);
      setState(140);
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
  enterRule(_localctx, 12, RQLParser::RuleSystempart);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(143);
    match(RQLParser::SYSTEM);
    setState(144);
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
  enterRule(_localctx, 14, RQLParser::RuleRational_se);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(149);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 15, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsFraction_proformaContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(146);
      fraction_rule();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsFloatContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(147);
      match(RQLParser::FLOAT);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsDecimalContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(148);
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
  enterRule(_localctx, 16, RQLParser::RuleRetention_from);
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
    setState(151);
    match(RQLParser::RETENTION);
    setState(152);
    antlrcpp::downCast<RetentionContext *>(_localctx)->capacity = match(RQLParser::DECIMAL);
    setState(154);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::DECIMAL) {
      setState(153);
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
  enterRule(_localctx, 18, RQLParser::RuleFraction_rule);

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
    setState(156);
    match(RQLParser::DECIMAL);
    setState(157);
    match(RQLParser::DIVIDE);
    setState(158);
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
  enterRule(_localctx, 20, RQLParser::RuleField_declaration);
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
    setState(160);
    match(RQLParser::ID);
    setState(161);
    field_type();
    setState(165);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::T__0) {
      setState(162);
      match(RQLParser::T__0);
      setState(163);
      antlrcpp::downCast<SingleDeclarationContext *>(_localctx)->type_size = match(RQLParser::DECIMAL);
      setState(164);
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
  enterRule(_localctx, 22, RQLParser::RuleField_type);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(173);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::BYTE_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeByteContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(167);
        match(RQLParser::BYTE_T);
        break;
      }

      case RQLParser::INTEGER_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeIntContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(168);
        match(RQLParser::INTEGER_T);
        break;
      }

      case RQLParser::UNSIGNED_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeUnsignedContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(169);
        match(RQLParser::UNSIGNED_T);
        break;
      }

      case RQLParser::FLOAT_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeFloatContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(170);
        match(RQLParser::FLOAT_T);
        break;
      }

      case RQLParser::DOUBLE_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeDoubleContext>(_localctx);
        enterOuterAlt(_localctx, 5);
        setState(171);
        match(RQLParser::DOUBLE_T);
        break;
      }

      case RQLParser::STRING_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeStringContext>(_localctx);
        enterOuterAlt(_localctx, 6);
        setState(172);
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
  enterRule(_localctx, 24, RQLParser::RuleSelect_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(184);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 20, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::SelectListFullscanContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(175);
      asterisk();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::SelectListContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(176);
      expression();
      setState(181);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == RQLParser::COMMA) {
        setState(177);
        match(RQLParser::COMMA);
        setState(178);
        expression();
        setState(183);
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
//----------------- FieldIDGeneratedContext ------------------------------------------------------------------

RQLParser::Gen_indexContext* RQLParser::FieldIDGeneratedContext::gen_index() {
  return getRuleContext<RQLParser::Gen_indexContext>(0);
}

tree::TerminalNode* RQLParser::FieldIDGeneratedContext::ID() {
  return getToken(RQLParser::ID, 0);
}

RQLParser::FieldIDGeneratedContext::FieldIDGeneratedContext(Field_idContext *ctx) { copyFrom(ctx); }

void RQLParser::FieldIDGeneratedContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFieldIDGenerated(this);
}
void RQLParser::FieldIDGeneratedContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFieldIDGenerated(this);
}
RQLParser::Field_idContext* RQLParser::field_id() {
  Field_idContext *_localctx = _tracker.createInstance<Field_idContext>(_ctx, getState());
  enterRule(_localctx, 26, RQLParser::RuleField_id);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(203);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 21, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(186);
      antlrcpp::downCast<FieldIDContext *>(_localctx)->column_name = match(RQLParser::ID);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDUnderlineContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(187);
      antlrcpp::downCast<FieldIDUnderlineContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(188);
      match(RQLParser::T__0);
      setState(189);
      match(RQLParser::UNDERLINE);
      setState(190);
      match(RQLParser::T__1);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDColumnNameContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(191);
      antlrcpp::downCast<FieldIDColumnNameContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(192);
      match(RQLParser::DOT);
      setState(193);
      antlrcpp::downCast<FieldIDColumnNameContext *>(_localctx)->column_name = match(RQLParser::ID);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDTableContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(194);
      antlrcpp::downCast<FieldIDTableContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(195);
      match(RQLParser::T__0);
      setState(196);
      antlrcpp::downCast<FieldIDTableContext *>(_localctx)->column_index = match(RQLParser::DECIMAL);
      setState(197);
      match(RQLParser::T__1);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDGeneratedContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(198);
      antlrcpp::downCast<FieldIDGeneratedContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(199);
      match(RQLParser::T__0);
      setState(200);
      gen_index(0);
      setState(201);
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
  enterRule(_localctx, 28, RQLParser::RuleUnary_op_expression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(209);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::BIT_NOT: {
        enterOuterAlt(_localctx, 1);
        setState(205);
        match(RQLParser::BIT_NOT);
        setState(206);
        expression();
        break;
      }

      case RQLParser::PLUS:
      case RQLParser::MINUS: {
        enterOuterAlt(_localctx, 2);
        setState(207);
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
        setState(208);
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
  enterRule(_localctx, 30, RQLParser::RuleAsterisk);
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
    setState(213);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::ID) {
      setState(211);
      match(RQLParser::ID);
      setState(212);
      match(RQLParser::DOT);
    }
    setState(215);
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
  enterRule(_localctx, 32, RQLParser::RuleExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(217);
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
  enterRule(_localctx, 34, RQLParser::RuleLogic);

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
    setState(219);
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
  size_t startState = 36;
  enterRecursionRule(_localctx, 36, RQLParser::RuleExpression_logic, precedence);

    

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

    setState(222);
    term_logic(0);
    _ctx->stop = _input->LT(-1);
    setState(232);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 25, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(230);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 24, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpAndContext>(_tracker.createInstance<Expression_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_logic);
          setState(224);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(225);
          match(RQLParser::AND_C);
          setState(226);
          expression_logic(4);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpOrContext>(_tracker.createInstance<Expression_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_logic);
          setState(227);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(228);
          match(RQLParser::OR_C);
          setState(229);
          expression_logic(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(234);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 25, _ctx);
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
  size_t startState = 38;
  enterRecursionRule(_localctx, 38, RQLParser::RuleTerm_logic, precedence);

    

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

    setState(236);
    expression_factor(0);
    _ctx->stop = _input->LT(-1);
    setState(258);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 27, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(256);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 26, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpEqContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(238);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(239);
          match(RQLParser::IS_EQ);
          setState(240);
          term_logic(8);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpNqContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(241);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(242);
          match(RQLParser::IS_NQ);
          setState(243);
          term_logic(7);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<ExpGrContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(244);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(245);
          match(RQLParser::IS_GR);
          setState(246);
          term_logic(6);
          break;
        }

        case 4: {
          auto newContext = _tracker.createInstance<ExpLsContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(247);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(248);
          match(RQLParser::IS_LS);
          setState(249);
          term_logic(5);
          break;
        }

        case 5: {
          auto newContext = _tracker.createInstance<ExpGeContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(250);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(251);
          match(RQLParser::IS_GE);
          setState(252);
          term_logic(4);
          break;
        }

        case 6: {
          auto newContext = _tracker.createInstance<ExpLeContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(253);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(254);
          match(RQLParser::IS_LE);
          setState(255);
          term_logic(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(260);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 27, _ctx);
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
  size_t startState = 40;
  enterRecursionRule(_localctx, 40, RQLParser::RuleExpression_factor, precedence);

    

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

    setState(262);
    term(0);
    _ctx->stop = _input->LT(-1);
    setState(272);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(270);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 28, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpPlusContext>(_tracker.createInstance<Expression_factorContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_factor);
          setState(264);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(265);
          match(RQLParser::PLUS);
          setState(266);
          expression_factor(4);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpMinusContext>(_tracker.createInstance<Expression_factorContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_factor);
          setState(267);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(268);
          match(RQLParser::MINUS);
          setState(269);
          expression_factor(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(274);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx);
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
//----------------- ExpGenIndexContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::ExpGenIndexContext::DOLLAR() {
  return getToken(RQLParser::DOLLAR, 0);
}

RQLParser::ExpGenIndexContext::ExpGenIndexContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpGenIndexContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpGenIndex(this);
}
void RQLParser::ExpGenIndexContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpGenIndex(this);
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
  size_t startState = 42;
  enterRecursionRule(_localctx, 42, RQLParser::RuleTerm, precedence);

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
    setState(296);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 32, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<ExpInContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(276);
      match(RQLParser::T__2);
      setState(277);
      expression_factor(0);
      setState(278);
      match(RQLParser::T__3);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<ExpFloatContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(281);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == RQLParser::MINUS) {
        setState(280);
        match(RQLParser::MINUS);
      }
      setState(283);
      match(RQLParser::FLOAT);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<ExpDecContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(285);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == RQLParser::MINUS) {
        setState(284);
        match(RQLParser::MINUS);
      }
      setState(287);
      match(RQLParser::DECIMAL);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<ExpStringContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(288);
      match(RQLParser::STRING);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<ExpUnaryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(289);
      unary_op_expression();
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<ExpFieldContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(290);
      field_id();
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<ExpAggContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(291);
      agregator();
      break;
    }

    case 8: {
      _localctx = _tracker.createInstance<ExpGenIndexContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(292);
      match(RQLParser::DOLLAR);
      break;
    }

    case 9: {
      _localctx = _tracker.createInstance<ExpFnCallContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(293);
      function_call();
      break;
    }

    case 10: {
      _localctx = _tracker.createInstance<ExpNotContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(294);
      match(RQLParser::NOT_C);
      setState(295);
      term(1);
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(306);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 34, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(304);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 33, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpMultContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm);
          setState(298);

          if (!(precpred(_ctx, 12))) throw FailedPredicateException(this, "precpred(_ctx, 12)");
          setState(299);
          match(RQLParser::STAR);
          setState(300);
          term(13);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpDivContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm);
          setState(301);

          if (!(precpred(_ctx, 11))) throw FailedPredicateException(this, "precpred(_ctx, 11)");
          setState(302);
          match(RQLParser::DIVIDE);
          setState(303);
          term(12);
          break;
        }

        default:
          break;
        } 
      }
      setState(308);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 34, _ctx);
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

std::vector<RQLParser::Stream_expressionContext *> RQLParser::SExpPlusContext::stream_expression() {
  return getRuleContexts<RQLParser::Stream_expressionContext>();
}

RQLParser::Stream_expressionContext* RQLParser::SExpPlusContext::stream_expression(size_t i) {
  return getRuleContext<RQLParser::Stream_expressionContext>(i);
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
//----------------- SExpTimeMoveContext ------------------------------------------------------------------

RQLParser::Stream_expressionContext* RQLParser::SExpTimeMoveContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
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
//----------------- SExpFactorContext ------------------------------------------------------------------

RQLParser::Stream_factorContext* RQLParser::SExpFactorContext::stream_factor() {
  return getRuleContext<RQLParser::Stream_factorContext>(0);
}

RQLParser::SExpFactorContext::SExpFactorContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

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

std::vector<RQLParser::Stream_expressionContext *> RQLParser::SExpHashContext::stream_expression() {
  return getRuleContexts<RQLParser::Stream_expressionContext>();
}

RQLParser::Stream_expressionContext* RQLParser::SExpHashContext::stream_expression(size_t i) {
  return getRuleContext<RQLParser::Stream_expressionContext>(i);
}

tree::TerminalNode* RQLParser::SExpHashContext::SHARP() {
  return getToken(RQLParser::SHARP, 0);
}

RQLParser::SExpHashContext::SExpHashContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

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

RQLParser::Stream_expressionContext* RQLParser::SExpModContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

tree::TerminalNode* RQLParser::SExpModContext::MOD() {
  return getToken(RQLParser::MOD, 0);
}

RQLParser::Rational_seContext* RQLParser::SExpModContext::rational_se() {
  return getRuleContext<RQLParser::Rational_seContext>(0);
}

RQLParser::SExpModContext::SExpModContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

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

RQLParser::Stream_expressionContext* RQLParser::SExpAgregate_proformaContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

tree::TerminalNode* RQLParser::SExpAgregate_proformaContext::DOT() {
  return getToken(RQLParser::DOT, 0);
}

RQLParser::AgregatorContext* RQLParser::SExpAgregate_proformaContext::agregator() {
  return getRuleContext<RQLParser::AgregatorContext>(0);
}

RQLParser::SExpAgregate_proformaContext::SExpAgregate_proformaContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

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

RQLParser::Stream_expressionContext* RQLParser::SExpAgseContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
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

RQLParser::SExpAgseContext::SExpAgseContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

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
//----------------- SExpMinusContext ------------------------------------------------------------------

RQLParser::Stream_expressionContext* RQLParser::SExpMinusContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
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
//----------------- SExpAndContext ------------------------------------------------------------------

RQLParser::Stream_expressionContext* RQLParser::SExpAndContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

tree::TerminalNode* RQLParser::SExpAndContext::AND() {
  return getToken(RQLParser::AND, 0);
}

RQLParser::Rational_seContext* RQLParser::SExpAndContext::rational_se() {
  return getRuleContext<RQLParser::Rational_seContext>(0);
}

RQLParser::SExpAndContext::SExpAndContext(Stream_expressionContext *ctx) { copyFrom(ctx); }

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

RQLParser::Stream_expressionContext* RQLParser::stream_expression() {
   return stream_expression(0);
}

RQLParser::Stream_expressionContext* RQLParser::stream_expression(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  RQLParser::Stream_expressionContext *_localctx = _tracker.createInstance<Stream_expressionContext>(_ctx, parentState);
  RQLParser::Stream_expressionContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 44;
  enterRecursionRule(_localctx, 44, RQLParser::RuleStream_expression, precedence);

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
    _localctx = _tracker.createInstance<SExpFactorContext>(_localctx);
    _ctx = _localctx;
    previousContext = _localctx;

    setState(310);
    stream_factor();
    _ctx->stop = _input->LT(-1);
    setState(345);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 37, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(343);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 36, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<SExpHashContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(312);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(313);
          match(RQLParser::SHARP);
          setState(314);
          stream_expression(4);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<SExpPlusContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(315);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(316);
          match(RQLParser::PLUS);
          setState(317);
          stream_expression(3);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<SExpAgseContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(318);

          if (!(precpred(_ctx, 9))) throw FailedPredicateException(this, "precpred(_ctx, 9)");
          setState(319);
          match(RQLParser::AT);
          setState(320);
          match(RQLParser::T__2);
          setState(321);
          antlrcpp::downCast<SExpAgseContext *>(_localctx)->step = match(RQLParser::DECIMAL);
          setState(322);
          match(RQLParser::COMMA);
          setState(324);
          _errHandler->sync(this);

          _la = _input->LA(1);
          if (_la == RQLParser::MINUS) {
            setState(323);
            match(RQLParser::MINUS);
          }
          setState(326);
          antlrcpp::downCast<SExpAgseContext *>(_localctx)->window = match(RQLParser::DECIMAL);
          setState(327);
          match(RQLParser::T__3);
          break;
        }

        case 4: {
          auto newContext = _tracker.createInstance<SExpAndContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(328);

          if (!(precpred(_ctx, 8))) throw FailedPredicateException(this, "precpred(_ctx, 8)");
          setState(329);
          match(RQLParser::AND);
          setState(330);
          rational_se();
          break;
        }

        case 5: {
          auto newContext = _tracker.createInstance<SExpModContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(331);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(332);
          match(RQLParser::MOD);
          setState(333);
          rational_se();
          break;
        }

        case 6: {
          auto newContext = _tracker.createInstance<SExpTimeMoveContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(334);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(335);
          match(RQLParser::IS_GR);
          setState(336);
          match(RQLParser::DECIMAL);
          break;
        }

        case 7: {
          auto newContext = _tracker.createInstance<SExpMinusContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(337);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(338);
          match(RQLParser::MINUS);
          setState(339);
          rational_se();
          break;
        }

        case 8: {
          auto newContext = _tracker.createInstance<SExpAgregate_proformaContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(340);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(341);
          match(RQLParser::DOT);
          setState(342);
          agregator();
          break;
        }

        default:
          break;
        } 
      }
      setState(347);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 37, _ctx);
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

RQLParser::Gen_indexContext* RQLParser::Stream_factorContext::gen_index() {
  return getRuleContext<RQLParser::Gen_indexContext>(0);
}

RQLParser::Stream_expressionContext* RQLParser::Stream_factorContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

RQLParser::Stream_fn_callContext* RQLParser::Stream_factorContext::stream_fn_call() {
  return getRuleContext<RQLParser::Stream_fn_callContext>(0);
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
  enterRule(_localctx, 46, RQLParser::RuleStream_factor);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(359);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 38, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(348);
      match(RQLParser::ID);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(349);
      match(RQLParser::ID);
      setState(350);
      match(RQLParser::T__0);
      setState(351);
      gen_index(0);
      setState(352);
      match(RQLParser::T__1);
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(354);
      match(RQLParser::T__2);
      setState(355);
      stream_expression(0);
      setState(356);
      match(RQLParser::T__3);
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(358);
      stream_fn_call();
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

//----------------- Gen_indexContext ------------------------------------------------------------------

RQLParser::Gen_indexContext::Gen_indexContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<RQLParser::Gen_indexContext *> RQLParser::Gen_indexContext::gen_index() {
  return getRuleContexts<RQLParser::Gen_indexContext>();
}

RQLParser::Gen_indexContext* RQLParser::Gen_indexContext::gen_index(size_t i) {
  return getRuleContext<RQLParser::Gen_indexContext>(i);
}

tree::TerminalNode* RQLParser::Gen_indexContext::DOLLAR() {
  return getToken(RQLParser::DOLLAR, 0);
}

tree::TerminalNode* RQLParser::Gen_indexContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

tree::TerminalNode* RQLParser::Gen_indexContext::STAR() {
  return getToken(RQLParser::STAR, 0);
}

tree::TerminalNode* RQLParser::Gen_indexContext::PLUS() {
  return getToken(RQLParser::PLUS, 0);
}

tree::TerminalNode* RQLParser::Gen_indexContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}


size_t RQLParser::Gen_indexContext::getRuleIndex() const {
  return RQLParser::RuleGen_index;
}

void RQLParser::Gen_indexContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterGen_index(this);
}

void RQLParser::Gen_indexContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitGen_index(this);
}


RQLParser::Gen_indexContext* RQLParser::gen_index() {
   return gen_index(0);
}

RQLParser::Gen_indexContext* RQLParser::gen_index(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  RQLParser::Gen_indexContext *_localctx = _tracker.createInstance<Gen_indexContext>(_ctx, parentState);
  RQLParser::Gen_indexContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 48;
  enterRecursionRule(_localctx, 48, RQLParser::RuleGen_index, precedence);

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
    setState(368);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::T__2: {
        setState(362);
        match(RQLParser::T__2);
        setState(363);
        gen_index(0);
        setState(364);
        match(RQLParser::T__3);
        break;
      }

      case RQLParser::DOLLAR: {
        setState(366);
        match(RQLParser::DOLLAR);
        break;
      }

      case RQLParser::DECIMAL: {
        setState(367);
        match(RQLParser::DECIMAL);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    _ctx->stop = _input->LT(-1);
    setState(378);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 41, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(376);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 40, _ctx)) {
        case 1: {
          _localctx = _tracker.createInstance<Gen_indexContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleGen_index);
          setState(370);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(371);
          match(RQLParser::STAR);
          setState(372);
          gen_index(6);
          break;
        }

        case 2: {
          _localctx = _tracker.createInstance<Gen_indexContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleGen_index);
          setState(373);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(374);
          _la = _input->LA(1);
          if (!(_la == RQLParser::PLUS

          || _la == RQLParser::MINUS)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(375);
          gen_index(5);
          break;
        }

        default:
          break;
        } 
      }
      setState(380);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 41, _ctx);
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
  enterRule(_localctx, 50, RQLParser::RuleAgregator);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(385);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::MIN: {
        _localctx = _tracker.createInstance<RQLParser::StreamMinContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(381);
        match(RQLParser::MIN);
        break;
      }

      case RQLParser::MAX: {
        _localctx = _tracker.createInstance<RQLParser::StreamMaxContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(382);
        match(RQLParser::MAX);
        break;
      }

      case RQLParser::AVG: {
        _localctx = _tracker.createInstance<RQLParser::StreamAvgContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(383);
        match(RQLParser::AVG);
        break;
      }

      case RQLParser::SUMC: {
        _localctx = _tracker.createInstance<RQLParser::StreamSumContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(384);
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

//----------------- Stream_fn_callContext ------------------------------------------------------------------

RQLParser::Stream_fn_callContext::Stream_fn_callContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

RQLParser::Stream_expressionContext* RQLParser::Stream_fn_callContext::stream_expression() {
  return getRuleContext<RQLParser::Stream_expressionContext>(0);
}

tree::TerminalNode* RQLParser::Stream_fn_callContext::MIN() {
  return getToken(RQLParser::MIN, 0);
}

tree::TerminalNode* RQLParser::Stream_fn_callContext::MAX() {
  return getToken(RQLParser::MAX, 0);
}

tree::TerminalNode* RQLParser::Stream_fn_callContext::AVG() {
  return getToken(RQLParser::AVG, 0);
}

tree::TerminalNode* RQLParser::Stream_fn_callContext::SUMC() {
  return getToken(RQLParser::SUMC, 0);
}


size_t RQLParser::Stream_fn_callContext::getRuleIndex() const {
  return RQLParser::RuleStream_fn_call;
}

void RQLParser::Stream_fn_callContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStream_fn_call(this);
}

void RQLParser::Stream_fn_callContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStream_fn_call(this);
}

RQLParser::Stream_fn_callContext* RQLParser::stream_fn_call() {
  Stream_fn_callContext *_localctx = _tracker.createInstance<Stream_fn_callContext>(_ctx, getState());
  enterRule(_localctx, 52, RQLParser::RuleStream_fn_call);
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
    setState(387);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 33776997205278720) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(388);
    match(RQLParser::T__2);
    setState(389);
    stream_expression(0);
    setState(390);
    match(RQLParser::T__3);
   
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

tree::TerminalNode* RQLParser::Function_callContext::TO_INTEGER_FN() {
  return getToken(RQLParser::TO_INTEGER_FN, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::TO_FLOAT_FN() {
  return getToken(RQLParser::TO_FLOAT_FN, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::TO_DOUBLE_FN() {
  return getToken(RQLParser::TO_DOUBLE_FN, 0);
}

std::vector<tree::TerminalNode *> RQLParser::Function_callContext::COMMA() {
  return getTokens(RQLParser::COMMA);
}

tree::TerminalNode* RQLParser::Function_callContext::COMMA(size_t i) {
  return getToken(RQLParser::COMMA, i);
}

tree::TerminalNode* RQLParser::Function_callContext::TO_STRING_FN() {
  return getToken(RQLParser::TO_STRING_FN, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::COLON() {
  return getToken(RQLParser::COLON, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
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
  enterRule(_localctx, 54, RQLParser::RuleFunction_call);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(413);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::T__4:
      case RQLParser::T__5:
      case RQLParser::T__6:
      case RQLParser::T__7:
      case RQLParser::T__8:
      case RQLParser::T__9:
      case RQLParser::T__10:
      case RQLParser::T__11:
      case RQLParser::T__12:
      case RQLParser::T__13:
      case RQLParser::T__14:
      case RQLParser::T__15:
      case RQLParser::T__16:
      case RQLParser::T__17:
      case RQLParser::T__18:
      case RQLParser::T__19:
      case RQLParser::T__20:
      case RQLParser::TO_INTEGER_FN:
      case RQLParser::TO_FLOAT_FN:
      case RQLParser::TO_DOUBLE_FN: {
        enterOuterAlt(_localctx, 1);
        setState(392);
        _la = _input->LA(1);
        if (!((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 1008806316535185376) != 0))) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(393);
        match(RQLParser::T__2);
        setState(394);
        expression_factor(0);
        setState(399);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == RQLParser::COMMA) {
          setState(395);
          match(RQLParser::COMMA);
          setState(396);
          expression_factor(0);
          setState(401);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        setState(402);
        match(RQLParser::T__3);
        break;
      }

      case RQLParser::TO_STRING_FN: {
        enterOuterAlt(_localctx, 2);
        setState(404);
        match(RQLParser::TO_STRING_FN);
        setState(405);
        match(RQLParser::T__2);
        setState(406);
        expression_factor(0);
        setState(409);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == RQLParser::COLON) {
          setState(407);
          match(RQLParser::COLON);
          setState(408);
          match(RQLParser::DECIMAL);
        }
        setState(411);
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

bool RQLParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 18: return expression_logicSempred(antlrcpp::downCast<Expression_logicContext *>(context), predicateIndex);
    case 19: return term_logicSempred(antlrcpp::downCast<Term_logicContext *>(context), predicateIndex);
    case 20: return expression_factorSempred(antlrcpp::downCast<Expression_factorContext *>(context), predicateIndex);
    case 21: return termSempred(antlrcpp::downCast<TermContext *>(context), predicateIndex);
    case 22: return stream_expressionSempred(antlrcpp::downCast<Stream_expressionContext *>(context), predicateIndex);
    case 24: return gen_indexSempred(antlrcpp::downCast<Gen_indexContext *>(context), predicateIndex);

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

bool RQLParser::stream_expressionSempred(Stream_expressionContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 12: return precpred(_ctx, 3);
    case 13: return precpred(_ctx, 2);
    case 14: return precpred(_ctx, 9);
    case 15: return precpred(_ctx, 8);
    case 16: return precpred(_ctx, 7);
    case 17: return precpred(_ctx, 6);
    case 18: return precpred(_ctx, 5);
    case 19: return precpred(_ctx, 4);

  default:
    break;
  }
  return true;
}

bool RQLParser::gen_indexSempred(Gen_indexContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 20: return precpred(_ctx, 5);
    case 21: return precpred(_ctx, 4);

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
