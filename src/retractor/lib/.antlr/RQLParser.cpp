
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
      "prog", "default_statement", "let_statement", "let_value", "var_statement", 
      "int_const", "compiler_option", "select_statement", "declare_statement", 
      "rule_statement", "dumppart", "systempart", "setpart", "rational_se", 
      "retention_from", "fraction_rule", "field_declaration", "field_type", 
      "select_list", "field_id", "unary_op_expression", "asterisk", "expression", 
      "logic", "expression_logic", "term_logic", "expression_factor", "term", 
      "stream_expression", "stream_factor", "gen_index", "agregator", "stream_fn_call", 
      "window_agg", "function_call"
    },
    std::vector<std::string>{
      "", "'['", "']'", "'('", "')'", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "", "", "'='", "'!='", "'>'", "'<'", "'>='", "'<='", 
      "'!'", "'||'", "'.'", "'_'", "'@'", "'#'", "'&'", "'%'", "'$'", "','", 
      "';'", "':'", "'::'", "'*'", "'/'", "'+'", "'-'", "'~'", "'|'", "'^'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "BYTE_T", "STRING_T", "UNSIGNED_T", "INTEGER_T", 
      "FLOAT_T", "DOUBLE_T", "SELECT", "STREAM", "FROM", "DECLARE", "RETENTION", 
      "FILE", "BINFILE", "TEXTFILE", "DEVICE", "STORAGE", "ROTATION", "SUBSTRAT", 
      "RULE", "DISPOSABLE", "ONESHOT", "HOLD", "TIMEOUT", "VOLATILE", "PERSISTENT", 
      "DEFAULT", "LET", "VAR", "ON", "WHEN", "DUMP", "SYSTEM", "SET", "DO", 
      "TO", "AND_C", "OR_C", "NOT_C", "MIN", "MAX", "AVG", "SUMC", "TYPE_PROFILE", 
      "STRING_PROFILE", "ID", "CONST_REF", "STRING", "FLOAT", "DECIMAL", 
      "REAL", "IS_EQ", "IS_NQ", "IS_GR", "IS_LS", "IS_GE", "IS_LE", "EXCLAMATION", 
      "DOUBLE_BAR", "DOT", "UNDERLINE", "AT", "SHARP", "AND", "MOD", "DOLLAR", 
      "COMMA", "SEMI", "COLON", "DOUBLE_COLON", "STAR", "DIVIDE", "PLUS", 
      "MINUS", "BIT_NOT", "BIT_OR", "BIT_XOR", "SPACE", "COMMENT", "LINE_COMMENT2"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,83,483,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,7,
  	28,2,29,7,29,2,30,7,30,2,31,7,31,2,32,7,32,2,33,7,33,2,34,7,34,1,0,1,
  	0,1,0,1,0,1,0,1,0,1,0,4,0,78,8,0,11,0,12,0,79,1,0,1,0,1,1,1,1,1,1,1,2,
  	1,2,1,2,1,2,1,2,1,3,3,3,93,8,3,1,3,1,3,1,3,1,3,1,3,3,3,100,8,3,1,4,1,
  	4,1,4,1,4,1,4,1,5,1,5,1,6,1,6,1,6,1,7,1,7,1,7,1,7,1,7,1,7,1,7,1,7,3,7,
  	120,8,7,1,7,1,7,1,7,1,7,3,7,126,8,7,1,7,3,7,129,8,7,1,7,3,7,132,8,7,1,
  	7,1,7,3,7,136,8,7,1,8,1,8,1,8,1,8,5,8,142,8,8,10,8,12,8,145,9,8,1,8,1,
  	8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,155,8,8,1,8,3,8,158,8,8,1,8,3,8,161,8,8,
  	1,8,3,8,164,8,8,1,8,3,8,167,8,8,1,9,1,9,1,9,1,9,1,9,1,9,1,9,1,9,1,9,1,
  	9,3,9,179,8,9,1,10,1,10,3,10,183,8,10,1,10,1,10,1,10,3,10,188,8,10,1,
  	10,1,10,1,10,3,10,193,8,10,1,11,1,11,1,11,1,12,1,12,1,12,1,12,1,12,1,
  	13,1,13,1,13,1,13,3,13,207,8,13,1,14,1,14,1,14,3,14,212,8,14,1,15,1,15,
  	1,15,1,15,1,16,1,16,1,16,1,16,1,16,1,16,3,16,224,8,16,1,17,1,17,1,17,
  	1,17,1,17,1,17,3,17,232,8,17,1,18,1,18,1,18,1,18,5,18,238,8,18,10,18,
  	12,18,241,9,18,3,18,243,8,18,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,
  	1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,1,19,3,19,263,8,19,1,20,
  	1,20,1,20,1,20,3,20,269,8,20,1,21,1,21,3,21,273,8,21,1,21,1,21,1,22,1,
  	22,1,23,1,23,1,24,1,24,1,24,1,24,1,24,1,24,1,24,1,24,1,24,5,24,290,8,
  	24,10,24,12,24,293,9,24,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,
  	1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,1,25,5,25,316,
  	8,25,10,25,12,25,319,9,25,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,
  	5,26,330,8,26,10,26,12,26,333,9,26,1,27,1,27,1,27,1,27,1,27,1,27,3,27,
  	341,8,27,1,27,1,27,3,27,345,8,27,1,27,1,27,3,27,349,8,27,1,27,1,27,1,
  	27,1,27,1,27,1,27,1,27,1,27,1,27,1,27,3,27,361,8,27,1,27,1,27,1,27,1,
  	27,1,27,1,27,1,27,1,27,1,27,5,27,372,8,27,10,27,12,27,375,9,27,1,28,1,
  	28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,3,
  	28,392,8,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,
  	28,1,28,1,28,1,28,1,28,1,28,1,28,5,28,412,8,28,10,28,12,28,415,9,28,1,
  	29,1,29,1,29,1,29,1,29,1,29,1,29,1,29,1,29,1,29,1,29,3,29,428,8,29,1,
  	30,1,30,1,30,1,30,1,30,1,30,1,30,1,30,3,30,438,8,30,1,30,1,30,1,30,1,
  	30,1,30,1,30,5,30,446,8,30,10,30,12,30,449,9,30,1,31,1,31,1,31,1,31,3,
  	31,455,8,31,1,32,1,32,1,32,1,32,1,32,1,33,1,33,1,33,1,33,1,33,1,33,1,
  	33,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,3,34,481,
  	8,34,1,34,0,6,48,50,52,54,56,60,35,0,2,4,6,8,10,12,14,16,18,20,22,24,
  	26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,0,8,
  	2,0,50,50,53,53,1,0,20,22,2,0,48,48,51,51,1,0,28,29,4,0,17,19,30,30,47,
  	47,49,49,1,0,16,19,1,0,76,77,1,0,43,46,539,0,77,1,0,0,0,2,83,1,0,0,0,
  	4,86,1,0,0,0,6,92,1,0,0,0,8,101,1,0,0,0,10,106,1,0,0,0,12,108,1,0,0,0,
  	14,111,1,0,0,0,16,137,1,0,0,0,18,168,1,0,0,0,20,180,1,0,0,0,22,194,1,
  	0,0,0,24,197,1,0,0,0,26,206,1,0,0,0,28,208,1,0,0,0,30,213,1,0,0,0,32,
  	217,1,0,0,0,34,231,1,0,0,0,36,242,1,0,0,0,38,262,1,0,0,0,40,268,1,0,0,
  	0,42,272,1,0,0,0,44,276,1,0,0,0,46,278,1,0,0,0,48,280,1,0,0,0,50,294,
  	1,0,0,0,52,320,1,0,0,0,54,360,1,0,0,0,56,376,1,0,0,0,58,427,1,0,0,0,60,
  	437,1,0,0,0,62,454,1,0,0,0,64,456,1,0,0,0,66,461,1,0,0,0,68,480,1,0,0,
  	0,70,78,3,14,7,0,71,78,3,16,8,0,72,78,3,12,6,0,73,78,3,2,1,0,74,78,3,
  	18,9,0,75,78,3,4,2,0,76,78,3,8,4,0,77,70,1,0,0,0,77,71,1,0,0,0,77,72,
  	1,0,0,0,77,73,1,0,0,0,77,74,1,0,0,0,77,75,1,0,0,0,77,76,1,0,0,0,78,79,
  	1,0,0,0,79,77,1,0,0,0,79,80,1,0,0,0,80,81,1,0,0,0,81,82,5,0,0,1,82,1,
  	1,0,0,0,83,84,5,30,0,0,84,85,5,28,0,0,85,3,1,0,0,0,86,87,5,31,0,0,87,
  	88,5,49,0,0,88,89,5,55,0,0,89,90,3,6,3,0,90,5,1,0,0,0,91,93,5,77,0,0,
  	92,91,1,0,0,0,92,93,1,0,0,0,93,99,1,0,0,0,94,100,5,53,0,0,95,100,5,52,
  	0,0,96,97,5,53,0,0,97,98,5,75,0,0,98,100,5,53,0,0,99,94,1,0,0,0,99,95,
  	1,0,0,0,99,96,1,0,0,0,100,7,1,0,0,0,101,102,5,32,0,0,102,103,5,49,0,0,
  	103,104,5,55,0,0,104,105,3,6,3,0,105,9,1,0,0,0,106,107,7,0,0,0,107,11,
  	1,0,0,0,108,109,7,1,0,0,109,110,7,2,0,0,110,13,1,0,0,0,111,112,5,11,0,
  	0,112,113,3,36,18,0,113,114,5,12,0,0,114,119,5,49,0,0,115,116,5,1,0,0,
  	116,117,3,10,5,0,117,118,5,2,0,0,118,120,1,0,0,0,119,115,1,0,0,0,119,
  	120,1,0,0,0,120,121,1,0,0,0,121,122,5,13,0,0,122,125,3,56,28,0,123,124,
  	5,16,0,0,124,126,5,51,0,0,125,123,1,0,0,0,125,126,1,0,0,0,126,128,1,0,
  	0,0,127,129,3,28,14,0,128,127,1,0,0,0,128,129,1,0,0,0,129,131,1,0,0,0,
  	130,132,7,3,0,0,131,130,1,0,0,0,131,132,1,0,0,0,132,135,1,0,0,0,133,134,
  	5,20,0,0,134,136,7,4,0,0,135,133,1,0,0,0,135,136,1,0,0,0,136,15,1,0,0,
  	0,137,138,5,14,0,0,138,143,3,32,16,0,139,140,5,70,0,0,140,142,3,32,16,
  	0,141,139,1,0,0,0,142,145,1,0,0,0,143,141,1,0,0,0,143,144,1,0,0,0,144,
  	146,1,0,0,0,145,143,1,0,0,0,146,147,5,12,0,0,147,148,5,49,0,0,148,149,
  	5,70,0,0,149,150,3,26,13,0,150,151,7,5,0,0,151,157,5,51,0,0,152,154,5,
  	27,0,0,153,155,5,77,0,0,154,153,1,0,0,0,154,155,1,0,0,0,155,156,1,0,0,
  	0,156,158,3,26,13,0,157,152,1,0,0,0,157,158,1,0,0,0,158,160,1,0,0,0,159,
  	161,5,24,0,0,160,159,1,0,0,0,160,161,1,0,0,0,161,163,1,0,0,0,162,164,
  	5,25,0,0,163,162,1,0,0,0,163,164,1,0,0,0,164,166,1,0,0,0,165,167,5,26,
  	0,0,166,165,1,0,0,0,166,167,1,0,0,0,167,17,1,0,0,0,168,169,5,23,0,0,169,
  	170,5,49,0,0,170,171,5,33,0,0,171,172,5,49,0,0,172,173,5,34,0,0,173,174,
  	3,46,23,0,174,178,5,38,0,0,175,179,3,20,10,0,176,179,3,22,11,0,177,179,
  	3,24,12,0,178,175,1,0,0,0,178,176,1,0,0,0,178,177,1,0,0,0,179,19,1,0,
  	0,0,180,182,5,35,0,0,181,183,5,77,0,0,182,181,1,0,0,0,182,183,1,0,0,0,
  	183,184,1,0,0,0,184,185,3,10,5,0,185,187,5,39,0,0,186,188,5,77,0,0,187,
  	186,1,0,0,0,187,188,1,0,0,0,188,189,1,0,0,0,189,192,3,10,5,0,190,191,
  	5,15,0,0,191,193,3,10,5,0,192,190,1,0,0,0,192,193,1,0,0,0,193,21,1,0,
  	0,0,194,195,5,36,0,0,195,196,5,51,0,0,196,23,1,0,0,0,197,198,5,37,0,0,
  	198,199,5,50,0,0,199,200,5,55,0,0,200,201,3,52,26,0,201,25,1,0,0,0,202,
  	207,3,30,15,0,203,207,5,52,0,0,204,207,5,53,0,0,205,207,5,50,0,0,206,
  	202,1,0,0,0,206,203,1,0,0,0,206,204,1,0,0,0,206,205,1,0,0,0,207,27,1,
  	0,0,0,208,209,5,15,0,0,209,211,3,10,5,0,210,212,3,10,5,0,211,210,1,0,
  	0,0,211,212,1,0,0,0,212,29,1,0,0,0,213,214,5,53,0,0,214,215,5,75,0,0,
  	215,216,5,53,0,0,216,31,1,0,0,0,217,218,5,49,0,0,218,223,3,34,17,0,219,
  	220,5,1,0,0,220,221,3,10,5,0,221,222,5,2,0,0,222,224,1,0,0,0,223,219,
  	1,0,0,0,223,224,1,0,0,0,224,33,1,0,0,0,225,232,5,5,0,0,226,232,5,8,0,
  	0,227,232,5,7,0,0,228,232,5,9,0,0,229,232,5,10,0,0,230,232,5,6,0,0,231,
  	225,1,0,0,0,231,226,1,0,0,0,231,227,1,0,0,0,231,228,1,0,0,0,231,229,1,
  	0,0,0,231,230,1,0,0,0,232,35,1,0,0,0,233,243,3,42,21,0,234,239,3,44,22,
  	0,235,236,5,70,0,0,236,238,3,44,22,0,237,235,1,0,0,0,238,241,1,0,0,0,
  	239,237,1,0,0,0,239,240,1,0,0,0,240,243,1,0,0,0,241,239,1,0,0,0,242,233,
  	1,0,0,0,242,234,1,0,0,0,243,37,1,0,0,0,244,263,5,49,0,0,245,246,5,49,
  	0,0,246,247,5,1,0,0,247,248,5,64,0,0,248,263,5,2,0,0,249,250,5,49,0,0,
  	250,251,5,63,0,0,251,263,5,49,0,0,252,253,5,49,0,0,253,254,5,1,0,0,254,
  	255,3,10,5,0,255,256,5,2,0,0,256,263,1,0,0,0,257,258,5,49,0,0,258,259,
  	5,1,0,0,259,260,3,60,30,0,260,261,5,2,0,0,261,263,1,0,0,0,262,244,1,0,
  	0,0,262,245,1,0,0,0,262,249,1,0,0,0,262,252,1,0,0,0,262,257,1,0,0,0,263,
  	39,1,0,0,0,264,265,5,78,0,0,265,269,3,54,27,0,266,267,7,6,0,0,267,269,
  	3,54,27,0,268,264,1,0,0,0,268,266,1,0,0,0,269,41,1,0,0,0,270,271,5,49,
  	0,0,271,273,5,63,0,0,272,270,1,0,0,0,272,273,1,0,0,0,273,274,1,0,0,0,
  	274,275,5,74,0,0,275,43,1,0,0,0,276,277,3,52,26,0,277,45,1,0,0,0,278,
  	279,3,48,24,0,279,47,1,0,0,0,280,281,6,24,-1,0,281,282,3,50,25,0,282,
  	291,1,0,0,0,283,284,10,3,0,0,284,285,5,40,0,0,285,290,3,48,24,4,286,287,
  	10,2,0,0,287,288,5,41,0,0,288,290,3,48,24,3,289,283,1,0,0,0,289,286,1,
  	0,0,0,290,293,1,0,0,0,291,289,1,0,0,0,291,292,1,0,0,0,292,49,1,0,0,0,
  	293,291,1,0,0,0,294,295,6,25,-1,0,295,296,3,52,26,0,296,317,1,0,0,0,297,
  	298,10,7,0,0,298,299,5,55,0,0,299,316,3,50,25,8,300,301,10,6,0,0,301,
  	302,5,56,0,0,302,316,3,50,25,7,303,304,10,5,0,0,304,305,5,57,0,0,305,
  	316,3,50,25,6,306,307,10,4,0,0,307,308,5,58,0,0,308,316,3,50,25,5,309,
  	310,10,3,0,0,310,311,5,59,0,0,311,316,3,50,25,4,312,313,10,2,0,0,313,
  	314,5,60,0,0,314,316,3,50,25,3,315,297,1,0,0,0,315,300,1,0,0,0,315,303,
  	1,0,0,0,315,306,1,0,0,0,315,309,1,0,0,0,315,312,1,0,0,0,316,319,1,0,0,
  	0,317,315,1,0,0,0,317,318,1,0,0,0,318,51,1,0,0,0,319,317,1,0,0,0,320,
  	321,6,26,-1,0,321,322,3,54,27,0,322,331,1,0,0,0,323,324,10,3,0,0,324,
  	325,5,76,0,0,325,330,3,52,26,4,326,327,10,2,0,0,327,328,5,77,0,0,328,
  	330,3,52,26,3,329,323,1,0,0,0,329,326,1,0,0,0,330,333,1,0,0,0,331,329,
  	1,0,0,0,331,332,1,0,0,0,332,53,1,0,0,0,333,331,1,0,0,0,334,335,6,27,-1,
  	0,335,336,5,3,0,0,336,337,3,52,26,0,337,338,5,4,0,0,338,361,1,0,0,0,339,
  	341,5,77,0,0,340,339,1,0,0,0,340,341,1,0,0,0,341,342,1,0,0,0,342,361,
  	5,52,0,0,343,345,5,77,0,0,344,343,1,0,0,0,344,345,1,0,0,0,345,346,1,0,
  	0,0,346,361,5,53,0,0,347,349,5,77,0,0,348,347,1,0,0,0,348,349,1,0,0,0,
  	349,350,1,0,0,0,350,361,5,50,0,0,351,361,5,51,0,0,352,361,3,40,20,0,353,
  	361,3,38,19,0,354,361,3,66,33,0,355,361,3,62,31,0,356,361,5,69,0,0,357,
  	361,3,68,34,0,358,359,5,42,0,0,359,361,3,54,27,1,360,334,1,0,0,0,360,
  	340,1,0,0,0,360,344,1,0,0,0,360,348,1,0,0,0,360,351,1,0,0,0,360,352,1,
  	0,0,0,360,353,1,0,0,0,360,354,1,0,0,0,360,355,1,0,0,0,360,356,1,0,0,0,
  	360,357,1,0,0,0,360,358,1,0,0,0,361,373,1,0,0,0,362,363,10,15,0,0,363,
  	364,5,80,0,0,364,372,3,54,27,15,365,366,10,14,0,0,366,367,5,74,0,0,367,
  	372,3,54,27,15,368,369,10,13,0,0,369,370,5,75,0,0,370,372,3,54,27,14,
  	371,362,1,0,0,0,371,365,1,0,0,0,371,368,1,0,0,0,372,375,1,0,0,0,373,371,
  	1,0,0,0,373,374,1,0,0,0,374,55,1,0,0,0,375,373,1,0,0,0,376,377,6,28,-1,
  	0,377,378,3,58,29,0,378,413,1,0,0,0,379,380,10,3,0,0,380,381,5,66,0,0,
  	381,412,3,56,28,4,382,383,10,2,0,0,383,384,5,76,0,0,384,412,3,56,28,3,
  	385,386,10,9,0,0,386,387,5,65,0,0,387,388,5,3,0,0,388,389,3,10,5,0,389,
  	391,5,70,0,0,390,392,5,77,0,0,391,390,1,0,0,0,391,392,1,0,0,0,392,393,
  	1,0,0,0,393,394,3,10,5,0,394,395,5,4,0,0,395,412,1,0,0,0,396,397,10,8,
  	0,0,397,398,5,67,0,0,398,412,3,26,13,0,399,400,10,7,0,0,400,401,5,68,
  	0,0,401,412,3,26,13,0,402,403,10,6,0,0,403,404,5,57,0,0,404,412,3,10,
  	5,0,405,406,10,5,0,0,406,407,5,77,0,0,407,412,3,26,13,0,408,409,10,4,
  	0,0,409,410,5,63,0,0,410,412,3,62,31,0,411,379,1,0,0,0,411,382,1,0,0,
  	0,411,385,1,0,0,0,411,396,1,0,0,0,411,399,1,0,0,0,411,402,1,0,0,0,411,
  	405,1,0,0,0,411,408,1,0,0,0,412,415,1,0,0,0,413,411,1,0,0,0,413,414,1,
  	0,0,0,414,57,1,0,0,0,415,413,1,0,0,0,416,428,5,49,0,0,417,418,5,49,0,
  	0,418,419,5,1,0,0,419,420,3,60,30,0,420,421,5,2,0,0,421,428,1,0,0,0,422,
  	423,5,3,0,0,423,424,3,56,28,0,424,425,5,4,0,0,425,428,1,0,0,0,426,428,
  	3,64,32,0,427,416,1,0,0,0,427,417,1,0,0,0,427,422,1,0,0,0,427,426,1,0,
  	0,0,428,59,1,0,0,0,429,430,6,30,-1,0,430,431,5,3,0,0,431,432,3,60,30,
  	0,432,433,5,4,0,0,433,438,1,0,0,0,434,438,5,69,0,0,435,438,5,53,0,0,436,
  	438,5,50,0,0,437,429,1,0,0,0,437,434,1,0,0,0,437,435,1,0,0,0,437,436,
  	1,0,0,0,438,447,1,0,0,0,439,440,10,6,0,0,440,441,5,74,0,0,441,446,3,60,
  	30,7,442,443,10,5,0,0,443,444,7,6,0,0,444,446,3,60,30,6,445,439,1,0,0,
  	0,445,442,1,0,0,0,446,449,1,0,0,0,447,445,1,0,0,0,447,448,1,0,0,0,448,
  	61,1,0,0,0,449,447,1,0,0,0,450,455,5,43,0,0,451,455,5,44,0,0,452,455,
  	5,45,0,0,453,455,5,46,0,0,454,450,1,0,0,0,454,451,1,0,0,0,454,452,1,0,
  	0,0,454,453,1,0,0,0,455,63,1,0,0,0,456,457,7,7,0,0,457,458,5,3,0,0,458,
  	459,3,56,28,0,459,460,5,4,0,0,460,65,1,0,0,0,461,462,7,7,0,0,462,463,
  	5,3,0,0,463,464,3,52,26,0,464,465,5,72,0,0,465,466,3,10,5,0,466,467,5,
  	4,0,0,467,67,1,0,0,0,468,469,5,49,0,0,469,470,5,3,0,0,470,471,3,52,26,
  	0,471,472,5,4,0,0,472,481,1,0,0,0,473,474,5,49,0,0,474,475,5,3,0,0,475,
  	476,3,52,26,0,476,477,5,72,0,0,477,478,3,10,5,0,478,479,5,4,0,0,479,481,
  	1,0,0,0,480,468,1,0,0,0,480,473,1,0,0,0,481,69,1,0,0,0,49,77,79,92,99,
  	119,125,128,131,135,143,154,157,160,163,166,178,182,187,192,206,211,223,
  	231,239,242,262,268,272,289,291,315,317,329,331,340,344,348,360,371,373,
  	391,411,413,427,437,445,447,454,480
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

std::vector<RQLParser::Default_statementContext *> RQLParser::ProgContext::default_statement() {
  return getRuleContexts<RQLParser::Default_statementContext>();
}

RQLParser::Default_statementContext* RQLParser::ProgContext::default_statement(size_t i) {
  return getRuleContext<RQLParser::Default_statementContext>(i);
}

std::vector<RQLParser::Rule_statementContext *> RQLParser::ProgContext::rule_statement() {
  return getRuleContexts<RQLParser::Rule_statementContext>();
}

RQLParser::Rule_statementContext* RQLParser::ProgContext::rule_statement(size_t i) {
  return getRuleContext<RQLParser::Rule_statementContext>(i);
}

std::vector<RQLParser::Let_statementContext *> RQLParser::ProgContext::let_statement() {
  return getRuleContexts<RQLParser::Let_statementContext>();
}

RQLParser::Let_statementContext* RQLParser::ProgContext::let_statement(size_t i) {
  return getRuleContext<RQLParser::Let_statementContext>(i);
}

std::vector<RQLParser::Var_statementContext *> RQLParser::ProgContext::var_statement() {
  return getRuleContexts<RQLParser::Var_statementContext>();
}

RQLParser::Var_statementContext* RQLParser::ProgContext::var_statement(size_t i) {
  return getRuleContext<RQLParser::Var_statementContext>(i);
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
    setState(77); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(77);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::SELECT: {
          setState(70);
          select_statement();
          break;
        }

        case RQLParser::DECLARE: {
          setState(71);
          declare_statement();
          break;
        }

        case RQLParser::STORAGE:
        case RQLParser::ROTATION:
        case RQLParser::SUBSTRAT: {
          setState(72);
          compiler_option();
          break;
        }

        case RQLParser::DEFAULT: {
          setState(73);
          default_statement();
          break;
        }

        case RQLParser::RULE: {
          setState(74);
          rule_statement();
          break;
        }

        case RQLParser::LET: {
          setState(75);
          let_statement();
          break;
        }

        case RQLParser::VAR: {
          setState(76);
          var_statement();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(79); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 7531939840) != 0));
    setState(81);
    match(RQLParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Default_statementContext ------------------------------------------------------------------

RQLParser::Default_statementContext::Default_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Default_statementContext::getRuleIndex() const {
  return RQLParser::RuleDefault_statement;
}

void RQLParser::Default_statementContext::copyFrom(Default_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- DefaultOptionContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::DefaultOptionContext::DEFAULT() {
  return getToken(RQLParser::DEFAULT, 0);
}

tree::TerminalNode* RQLParser::DefaultOptionContext::VOLATILE() {
  return getToken(RQLParser::VOLATILE, 0);
}

RQLParser::DefaultOptionContext::DefaultOptionContext(Default_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::DefaultOptionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDefaultOption(this);
}
void RQLParser::DefaultOptionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDefaultOption(this);
}
RQLParser::Default_statementContext* RQLParser::default_statement() {
  Default_statementContext *_localctx = _tracker.createInstance<Default_statementContext>(_ctx, getState());
  enterRule(_localctx, 2, RQLParser::RuleDefault_statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::DefaultOptionContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(83);
    match(RQLParser::DEFAULT);
    setState(84);
    match(RQLParser::VOLATILE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Let_statementContext ------------------------------------------------------------------

RQLParser::Let_statementContext::Let_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Let_statementContext::getRuleIndex() const {
  return RQLParser::RuleLet_statement;
}

void RQLParser::Let_statementContext::copyFrom(Let_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- LetContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::LetContext::LET() {
  return getToken(RQLParser::LET, 0);
}

tree::TerminalNode* RQLParser::LetContext::IS_EQ() {
  return getToken(RQLParser::IS_EQ, 0);
}

RQLParser::Let_valueContext* RQLParser::LetContext::let_value() {
  return getRuleContext<RQLParser::Let_valueContext>(0);
}

tree::TerminalNode* RQLParser::LetContext::ID() {
  return getToken(RQLParser::ID, 0);
}

RQLParser::LetContext::LetContext(Let_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::LetContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLet(this);
}
void RQLParser::LetContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLet(this);
}
RQLParser::Let_statementContext* RQLParser::let_statement() {
  Let_statementContext *_localctx = _tracker.createInstance<Let_statementContext>(_ctx, getState());
  enterRule(_localctx, 4, RQLParser::RuleLet_statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::LetContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(86);
    match(RQLParser::LET);
    setState(87);
    antlrcpp::downCast<LetContext *>(_localctx)->name = match(RQLParser::ID);
    setState(88);
    match(RQLParser::IS_EQ);
    setState(89);
    let_value();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Let_valueContext ------------------------------------------------------------------

RQLParser::Let_valueContext::Let_valueContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> RQLParser::Let_valueContext::DECIMAL() {
  return getTokens(RQLParser::DECIMAL);
}

tree::TerminalNode* RQLParser::Let_valueContext::DECIMAL(size_t i) {
  return getToken(RQLParser::DECIMAL, i);
}

tree::TerminalNode* RQLParser::Let_valueContext::FLOAT() {
  return getToken(RQLParser::FLOAT, 0);
}

tree::TerminalNode* RQLParser::Let_valueContext::DIVIDE() {
  return getToken(RQLParser::DIVIDE, 0);
}

tree::TerminalNode* RQLParser::Let_valueContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}


size_t RQLParser::Let_valueContext::getRuleIndex() const {
  return RQLParser::RuleLet_value;
}

void RQLParser::Let_valueContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLet_value(this);
}

void RQLParser::Let_valueContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLet_value(this);
}

RQLParser::Let_valueContext* RQLParser::let_value() {
  Let_valueContext *_localctx = _tracker.createInstance<Let_valueContext>(_ctx, getState());
  enterRule(_localctx, 6, RQLParser::RuleLet_value);
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
    setState(92);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::MINUS) {
      setState(91);
      antlrcpp::downCast<Let_valueContext *>(_localctx)->sign = match(RQLParser::MINUS);
    }
    setState(99);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 3, _ctx)) {
    case 1: {
      setState(94);
      match(RQLParser::DECIMAL);
      break;
    }

    case 2: {
      setState(95);
      match(RQLParser::FLOAT);
      break;
    }

    case 3: {
      setState(96);
      antlrcpp::downCast<Let_valueContext *>(_localctx)->nom = match(RQLParser::DECIMAL);
      setState(97);
      match(RQLParser::DIVIDE);
      setState(98);
      antlrcpp::downCast<Let_valueContext *>(_localctx)->den = match(RQLParser::DECIMAL);
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

//----------------- Var_statementContext ------------------------------------------------------------------

RQLParser::Var_statementContext::Var_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t RQLParser::Var_statementContext::getRuleIndex() const {
  return RQLParser::RuleVar_statement;
}

void RQLParser::Var_statementContext::copyFrom(Var_statementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- VarContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::VarContext::VAR() {
  return getToken(RQLParser::VAR, 0);
}

tree::TerminalNode* RQLParser::VarContext::IS_EQ() {
  return getToken(RQLParser::IS_EQ, 0);
}

RQLParser::Let_valueContext* RQLParser::VarContext::let_value() {
  return getRuleContext<RQLParser::Let_valueContext>(0);
}

tree::TerminalNode* RQLParser::VarContext::ID() {
  return getToken(RQLParser::ID, 0);
}

RQLParser::VarContext::VarContext(Var_statementContext *ctx) { copyFrom(ctx); }

void RQLParser::VarContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterVar(this);
}
void RQLParser::VarContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitVar(this);
}
RQLParser::Var_statementContext* RQLParser::var_statement() {
  Var_statementContext *_localctx = _tracker.createInstance<Var_statementContext>(_ctx, getState());
  enterRule(_localctx, 8, RQLParser::RuleVar_statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    _localctx = _tracker.createInstance<RQLParser::VarContext>(_localctx);
    enterOuterAlt(_localctx, 1);
    setState(101);
    match(RQLParser::VAR);
    setState(102);
    antlrcpp::downCast<VarContext *>(_localctx)->name = match(RQLParser::ID);
    setState(103);
    match(RQLParser::IS_EQ);
    setState(104);
    let_value();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Int_constContext ------------------------------------------------------------------

RQLParser::Int_constContext::Int_constContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::Int_constContext::DECIMAL() {
  return getToken(RQLParser::DECIMAL, 0);
}

tree::TerminalNode* RQLParser::Int_constContext::CONST_REF() {
  return getToken(RQLParser::CONST_REF, 0);
}


size_t RQLParser::Int_constContext::getRuleIndex() const {
  return RQLParser::RuleInt_const;
}

void RQLParser::Int_constContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterInt_const(this);
}

void RQLParser::Int_constContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitInt_const(this);
}

RQLParser::Int_constContext* RQLParser::int_const() {
  Int_constContext *_localctx = _tracker.createInstance<Int_constContext>(_ctx, getState());
  enterRule(_localctx, 10, RQLParser::RuleInt_const);
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
    setState(106);
    _la = _input->LA(1);
    if (!(_la == RQLParser::CONST_REF

    || _la == RQLParser::DECIMAL)) {
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
  enterRule(_localctx, 12, RQLParser::RuleCompiler_option);
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
    setState(108);
    antlrcpp::downCast<CoptionContext *>(_localctx)->directive = _input->LT(1);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 7340032) != 0))) {
      antlrcpp::downCast<CoptionContext *>(_localctx)->directive = _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(109);
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

std::vector<tree::TerminalNode *> RQLParser::SelectContext::ID() {
  return getTokens(RQLParser::ID);
}

tree::TerminalNode* RQLParser::SelectContext::ID(size_t i) {
  return getToken(RQLParser::ID, i);
}

tree::TerminalNode* RQLParser::SelectContext::FILE() {
  return getToken(RQLParser::FILE, 0);
}

RQLParser::Retention_fromContext* RQLParser::SelectContext::retention_from() {
  return getRuleContext<RQLParser::Retention_fromContext>(0);
}

tree::TerminalNode* RQLParser::SelectContext::STORAGE() {
  return getToken(RQLParser::STORAGE, 0);
}

RQLParser::Int_constContext* RQLParser::SelectContext::int_const() {
  return getRuleContext<RQLParser::Int_constContext>(0);
}

tree::TerminalNode* RQLParser::SelectContext::STRING() {
  return getToken(RQLParser::STRING, 0);
}

tree::TerminalNode* RQLParser::SelectContext::VOLATILE() {
  return getToken(RQLParser::VOLATILE, 0);
}

tree::TerminalNode* RQLParser::SelectContext::PERSISTENT() {
  return getToken(RQLParser::PERSISTENT, 0);
}

tree::TerminalNode* RQLParser::SelectContext::TYPE_PROFILE() {
  return getToken(RQLParser::TYPE_PROFILE, 0);
}

tree::TerminalNode* RQLParser::SelectContext::DEFAULT() {
  return getToken(RQLParser::DEFAULT, 0);
}

tree::TerminalNode* RQLParser::SelectContext::DEVICE() {
  return getToken(RQLParser::DEVICE, 0);
}

tree::TerminalNode* RQLParser::SelectContext::BINFILE() {
  return getToken(RQLParser::BINFILE, 0);
}

tree::TerminalNode* RQLParser::SelectContext::TEXTFILE() {
  return getToken(RQLParser::TEXTFILE, 0);
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
  enterRule(_localctx, 14, RQLParser::RuleSelect_statement);
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
    setState(111);
    match(RQLParser::SELECT);
    setState(112);
    select_list();
    setState(113);
    match(RQLParser::STREAM);
    setState(114);
    antlrcpp::downCast<SelectContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(119);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::T__0) {
      setState(115);
      match(RQLParser::T__0);
      setState(116);
      antlrcpp::downCast<SelectContext *>(_localctx)->gen_size = int_const();
      setState(117);
      match(RQLParser::T__1);
    }
    setState(121);
    match(RQLParser::FROM);
    setState(122);
    stream_expression(0);
    setState(125);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::FILE) {
      setState(123);
      match(RQLParser::FILE);
      setState(124);
      antlrcpp::downCast<SelectContext *>(_localctx)->file_name = match(RQLParser::STRING);
    }
    setState(128);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::RETENTION) {
      setState(127);
      retention_from();
    }
    setState(131);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::VOLATILE

    || _la == RQLParser::PERSISTENT) {
      setState(130);
      _la = _input->LA(1);
      if (!(_la == RQLParser::VOLATILE

      || _la == RQLParser::PERSISTENT)) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
    }
    setState(135);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 8, _ctx)) {
    case 1: {
      setState(133);
      match(RQLParser::STORAGE);
      setState(134);
      antlrcpp::downCast<SelectContext *>(_localctx)->type_name = _input->LT(1);
      _la = _input->LA(1);
      if (!((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 703688516435968) != 0))) {
        antlrcpp::downCast<SelectContext *>(_localctx)->type_name = _errHandler->recoverInline(this);
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

std::vector<RQLParser::Rational_seContext *> RQLParser::DeclareContext::rational_se() {
  return getRuleContexts<RQLParser::Rational_seContext>();
}

RQLParser::Rational_seContext* RQLParser::DeclareContext::rational_se(size_t i) {
  return getRuleContext<RQLParser::Rational_seContext>(i);
}

tree::TerminalNode* RQLParser::DeclareContext::ID() {
  return getToken(RQLParser::ID, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::STRING() {
  return getToken(RQLParser::STRING, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::BINFILE() {
  return getToken(RQLParser::BINFILE, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::TEXTFILE() {
  return getToken(RQLParser::TEXTFILE, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::DEVICE() {
  return getToken(RQLParser::DEVICE, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::FILE() {
  return getToken(RQLParser::FILE, 0);
}

tree::TerminalNode* RQLParser::DeclareContext::TIMEOUT() {
  return getToken(RQLParser::TIMEOUT, 0);
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

tree::TerminalNode* RQLParser::DeclareContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
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
  enterRule(_localctx, 16, RQLParser::RuleDeclare_statement);
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
    setState(137);
    match(RQLParser::DECLARE);
    setState(138);
    field_declaration();
    setState(143);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(139);
      match(RQLParser::COMMA);
      setState(140);
      field_declaration();
      setState(145);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(146);
    match(RQLParser::STREAM);
    setState(147);
    antlrcpp::downCast<DeclareContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(148);
    match(RQLParser::COMMA);
    setState(149);
    rational_se();
    setState(150);
    antlrcpp::downCast<DeclareContext *>(_localctx)->kind = _input->LT(1);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 983040) != 0))) {
      antlrcpp::downCast<DeclareContext *>(_localctx)->kind = _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(151);
    antlrcpp::downCast<DeclareContext *>(_localctx)->file_name = match(RQLParser::STRING);
    setState(157);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::TIMEOUT) {
      setState(152);
      match(RQLParser::TIMEOUT);
      setState(154);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == RQLParser::MINUS) {
        setState(153);
        antlrcpp::downCast<DeclareContext *>(_localctx)->timeout_sign = match(RQLParser::MINUS);
      }
      setState(156);
      antlrcpp::downCast<DeclareContext *>(_localctx)->timeout_value = rational_se();
    }
    setState(160);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::DISPOSABLE) {
      setState(159);
      match(RQLParser::DISPOSABLE);
    }
    setState(163);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::ONESHOT) {
      setState(162);
      match(RQLParser::ONESHOT);
    }
    setState(166);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::HOLD) {
      setState(165);
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

RQLParser::SetpartContext* RQLParser::RulezContext::setpart() {
  return getRuleContext<RQLParser::SetpartContext>(0);
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
  enterRule(_localctx, 18, RQLParser::RuleRule_statement);

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
    setState(168);
    match(RQLParser::RULE);
    setState(169);
    antlrcpp::downCast<RulezContext *>(_localctx)->name = match(RQLParser::ID);
    setState(170);
    match(RQLParser::ON);
    setState(171);
    antlrcpp::downCast<RulezContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(172);
    match(RQLParser::WHEN);
    setState(173);
    logic();
    setState(174);
    match(RQLParser::DO);
    setState(178);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::DUMP: {
        setState(175);
        dumppart();
        break;
      }

      case RQLParser::SYSTEM: {
        setState(176);
        systempart();
        break;
      }

      case RQLParser::SET: {
        setState(177);
        setpart();
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

std::vector<RQLParser::Int_constContext *> RQLParser::DumppartContext::int_const() {
  return getRuleContexts<RQLParser::Int_constContext>();
}

RQLParser::Int_constContext* RQLParser::DumppartContext::int_const(size_t i) {
  return getRuleContext<RQLParser::Int_constContext>(i);
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
  enterRule(_localctx, 20, RQLParser::RuleDumppart);
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
    setState(180);
    match(RQLParser::DUMP);
    setState(182);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::MINUS) {
      setState(181);
      match(RQLParser::MINUS);
    }
    setState(184);
    antlrcpp::downCast<DumppartContext *>(_localctx)->step_back = int_const();
    setState(185);
    match(RQLParser::TO);
    setState(187);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::MINUS) {
      setState(186);
      match(RQLParser::MINUS);
    }
    setState(189);
    antlrcpp::downCast<DumppartContext *>(_localctx)->step_forward = int_const();
    setState(192);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::RETENTION) {
      setState(190);
      match(RQLParser::RETENTION);
      setState(191);
      antlrcpp::downCast<DumppartContext *>(_localctx)->rule_retnetion = int_const();
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
  enterRule(_localctx, 22, RQLParser::RuleSystempart);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(194);
    match(RQLParser::SYSTEM);
    setState(195);
    antlrcpp::downCast<SystempartContext *>(_localctx)->syscmd = match(RQLParser::STRING);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SetpartContext ------------------------------------------------------------------

RQLParser::SetpartContext::SetpartContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::SetpartContext::SET() {
  return getToken(RQLParser::SET, 0);
}

tree::TerminalNode* RQLParser::SetpartContext::IS_EQ() {
  return getToken(RQLParser::IS_EQ, 0);
}

tree::TerminalNode* RQLParser::SetpartContext::CONST_REF() {
  return getToken(RQLParser::CONST_REF, 0);
}

RQLParser::Expression_factorContext* RQLParser::SetpartContext::expression_factor() {
  return getRuleContext<RQLParser::Expression_factorContext>(0);
}


size_t RQLParser::SetpartContext::getRuleIndex() const {
  return RQLParser::RuleSetpart;
}

void RQLParser::SetpartContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSetpart(this);
}

void RQLParser::SetpartContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSetpart(this);
}

RQLParser::SetpartContext* RQLParser::setpart() {
  SetpartContext *_localctx = _tracker.createInstance<SetpartContext>(_ctx, getState());
  enterRule(_localctx, 24, RQLParser::RuleSetpart);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(197);
    match(RQLParser::SET);
    setState(198);
    antlrcpp::downCast<SetpartContext *>(_localctx)->target = match(RQLParser::CONST_REF);
    setState(199);
    match(RQLParser::IS_EQ);
    setState(200);
    antlrcpp::downCast<SetpartContext *>(_localctx)->value = expression_factor(0);
   
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
//----------------- RationalAsConstContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::RationalAsConstContext::CONST_REF() {
  return getToken(RQLParser::CONST_REF, 0);
}

RQLParser::RationalAsConstContext::RationalAsConstContext(Rational_seContext *ctx) { copyFrom(ctx); }

void RQLParser::RationalAsConstContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRationalAsConst(this);
}
void RQLParser::RationalAsConstContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRationalAsConst(this);
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
  enterRule(_localctx, 26, RQLParser::RuleRational_se);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(206);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 19, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsFraction_proformaContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(202);
      fraction_rule();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsFloatContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(203);
      match(RQLParser::FLOAT);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsDecimalContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(204);
      match(RQLParser::DECIMAL);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsConstContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(205);
      match(RQLParser::CONST_REF);
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

std::vector<RQLParser::Int_constContext *> RQLParser::RetentionContext::int_const() {
  return getRuleContexts<RQLParser::Int_constContext>();
}

RQLParser::Int_constContext* RQLParser::RetentionContext::int_const(size_t i) {
  return getRuleContext<RQLParser::Int_constContext>(i);
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
  enterRule(_localctx, 28, RQLParser::RuleRetention_from);
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
    setState(208);
    match(RQLParser::RETENTION);
    setState(209);
    antlrcpp::downCast<RetentionContext *>(_localctx)->capacity = int_const();
    setState(211);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::CONST_REF

    || _la == RQLParser::DECIMAL) {
      setState(210);
      antlrcpp::downCast<RetentionContext *>(_localctx)->segments = int_const();
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
  enterRule(_localctx, 30, RQLParser::RuleFraction_rule);

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
    setState(213);
    match(RQLParser::DECIMAL);
    setState(214);
    match(RQLParser::DIVIDE);
    setState(215);
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

RQLParser::Int_constContext* RQLParser::SingleDeclarationContext::int_const() {
  return getRuleContext<RQLParser::Int_constContext>(0);
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
  enterRule(_localctx, 32, RQLParser::RuleField_declaration);
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
    setState(217);
    match(RQLParser::ID);
    setState(218);
    field_type();
    setState(223);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::T__0) {
      setState(219);
      match(RQLParser::T__0);
      setState(220);
      antlrcpp::downCast<SingleDeclarationContext *>(_localctx)->type_size = int_const();
      setState(221);
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
  enterRule(_localctx, 34, RQLParser::RuleField_type);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(231);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::BYTE_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeByteContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(225);
        match(RQLParser::BYTE_T);
        break;
      }

      case RQLParser::INTEGER_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeIntContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(226);
        match(RQLParser::INTEGER_T);
        break;
      }

      case RQLParser::UNSIGNED_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeUnsignedContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(227);
        match(RQLParser::UNSIGNED_T);
        break;
      }

      case RQLParser::FLOAT_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeFloatContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(228);
        match(RQLParser::FLOAT_T);
        break;
      }

      case RQLParser::DOUBLE_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeDoubleContext>(_localctx);
        enterOuterAlt(_localctx, 5);
        setState(229);
        match(RQLParser::DOUBLE_T);
        break;
      }

      case RQLParser::STRING_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeStringContext>(_localctx);
        enterOuterAlt(_localctx, 6);
        setState(230);
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
  enterRule(_localctx, 36, RQLParser::RuleSelect_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(242);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 24, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::SelectListFullscanContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(233);
      asterisk();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::SelectListContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(234);
      expression();
      setState(239);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == RQLParser::COMMA) {
        setState(235);
        match(RQLParser::COMMA);
        setState(236);
        expression();
        setState(241);
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

RQLParser::Int_constContext* RQLParser::FieldIDTableContext::int_const() {
  return getRuleContext<RQLParser::Int_constContext>(0);
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
  enterRule(_localctx, 38, RQLParser::RuleField_id);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(262);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 25, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(244);
      antlrcpp::downCast<FieldIDContext *>(_localctx)->column_name = match(RQLParser::ID);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDUnderlineContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(245);
      antlrcpp::downCast<FieldIDUnderlineContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(246);
      match(RQLParser::T__0);
      setState(247);
      match(RQLParser::UNDERLINE);
      setState(248);
      match(RQLParser::T__1);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDColumnNameContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(249);
      antlrcpp::downCast<FieldIDColumnNameContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(250);
      match(RQLParser::DOT);
      setState(251);
      antlrcpp::downCast<FieldIDColumnNameContext *>(_localctx)->column_name = match(RQLParser::ID);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDTableContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(252);
      antlrcpp::downCast<FieldIDTableContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(253);
      match(RQLParser::T__0);
      setState(254);
      antlrcpp::downCast<FieldIDTableContext *>(_localctx)->column_index = int_const();
      setState(255);
      match(RQLParser::T__1);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDGeneratedContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(257);
      antlrcpp::downCast<FieldIDGeneratedContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(258);
      match(RQLParser::T__0);
      setState(259);
      gen_index(0);
      setState(260);
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

RQLParser::TermContext* RQLParser::Unary_op_expressionContext::term() {
  return getRuleContext<RQLParser::TermContext>(0);
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
  enterRule(_localctx, 40, RQLParser::RuleUnary_op_expression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(268);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::BIT_NOT: {
        enterOuterAlt(_localctx, 1);
        setState(264);
        match(RQLParser::BIT_NOT);
        setState(265);
        term(0);
        break;
      }

      case RQLParser::PLUS:
      case RQLParser::MINUS: {
        enterOuterAlt(_localctx, 2);
        setState(266);
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
        setState(267);
        term(0);
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
  enterRule(_localctx, 42, RQLParser::RuleAsterisk);
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
    setState(272);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::ID) {
      setState(270);
      match(RQLParser::ID);
      setState(271);
      match(RQLParser::DOT);
    }
    setState(274);
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
  enterRule(_localctx, 44, RQLParser::RuleExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(276);
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
  enterRule(_localctx, 46, RQLParser::RuleLogic);

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
    setState(278);
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
  size_t startState = 48;
  enterRecursionRule(_localctx, 48, RQLParser::RuleExpression_logic, precedence);

    

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

    setState(281);
    term_logic(0);
    _ctx->stop = _input->LT(-1);
    setState(291);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(289);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 28, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpAndContext>(_tracker.createInstance<Expression_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_logic);
          setState(283);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(284);
          match(RQLParser::AND_C);
          setState(285);
          expression_logic(4);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpOrContext>(_tracker.createInstance<Expression_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_logic);
          setState(286);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(287);
          match(RQLParser::OR_C);
          setState(288);
          expression_logic(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(293);
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
  size_t startState = 50;
  enterRecursionRule(_localctx, 50, RQLParser::RuleTerm_logic, precedence);

    

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

    setState(295);
    expression_factor(0);
    _ctx->stop = _input->LT(-1);
    setState(317);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 31, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(315);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 30, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpEqContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(297);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(298);
          match(RQLParser::IS_EQ);
          setState(299);
          term_logic(8);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpNqContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(300);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(301);
          match(RQLParser::IS_NQ);
          setState(302);
          term_logic(7);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<ExpGrContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(303);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(304);
          match(RQLParser::IS_GR);
          setState(305);
          term_logic(6);
          break;
        }

        case 4: {
          auto newContext = _tracker.createInstance<ExpLsContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(306);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(307);
          match(RQLParser::IS_LS);
          setState(308);
          term_logic(5);
          break;
        }

        case 5: {
          auto newContext = _tracker.createInstance<ExpGeContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(309);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(310);
          match(RQLParser::IS_GE);
          setState(311);
          term_logic(4);
          break;
        }

        case 6: {
          auto newContext = _tracker.createInstance<ExpLeContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(312);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(313);
          match(RQLParser::IS_LE);
          setState(314);
          term_logic(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(319);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 31, _ctx);
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
  size_t startState = 52;
  enterRecursionRule(_localctx, 52, RQLParser::RuleExpression_factor, precedence);

    

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

    setState(321);
    term(0);
    _ctx->stop = _input->LT(-1);
    setState(331);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 33, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(329);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 32, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpPlusContext>(_tracker.createInstance<Expression_factorContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_factor);
          setState(323);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(324);
          match(RQLParser::PLUS);
          setState(325);
          expression_factor(4);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpMinusContext>(_tracker.createInstance<Expression_factorContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_factor);
          setState(326);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(327);
          match(RQLParser::MINUS);
          setState(328);
          expression_factor(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(333);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 33, _ctx);
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
//----------------- ExpWindowAggContext ------------------------------------------------------------------

RQLParser::Window_aggContext* RQLParser::ExpWindowAggContext::window_agg() {
  return getRuleContext<RQLParser::Window_aggContext>(0);
}

RQLParser::ExpWindowAggContext::ExpWindowAggContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpWindowAggContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpWindowAgg(this);
}
void RQLParser::ExpWindowAggContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpWindowAgg(this);
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
//----------------- ExpPowContext ------------------------------------------------------------------

std::vector<RQLParser::TermContext *> RQLParser::ExpPowContext::term() {
  return getRuleContexts<RQLParser::TermContext>();
}

RQLParser::TermContext* RQLParser::ExpPowContext::term(size_t i) {
  return getRuleContext<RQLParser::TermContext>(i);
}

tree::TerminalNode* RQLParser::ExpPowContext::BIT_XOR() {
  return getToken(RQLParser::BIT_XOR, 0);
}

RQLParser::ExpPowContext::ExpPowContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpPowContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpPow(this);
}
void RQLParser::ExpPowContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpPow(this);
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
//----------------- ExpConstContext ------------------------------------------------------------------

tree::TerminalNode* RQLParser::ExpConstContext::CONST_REF() {
  return getToken(RQLParser::CONST_REF, 0);
}

tree::TerminalNode* RQLParser::ExpConstContext::MINUS() {
  return getToken(RQLParser::MINUS, 0);
}

RQLParser::ExpConstContext::ExpConstContext(TermContext *ctx) { copyFrom(ctx); }

void RQLParser::ExpConstContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpConst(this);
}
void RQLParser::ExpConstContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpConst(this);
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

RQLParser::TermContext* RQLParser::term() {
   return term(0);
}

RQLParser::TermContext* RQLParser::term(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  RQLParser::TermContext *_localctx = _tracker.createInstance<TermContext>(_ctx, parentState);
  RQLParser::TermContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 54;
  enterRecursionRule(_localctx, 54, RQLParser::RuleTerm, precedence);

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
    setState(360);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 37, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<ExpInContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(335);
      match(RQLParser::T__2);
      setState(336);
      expression_factor(0);
      setState(337);
      match(RQLParser::T__3);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<ExpFloatContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(340);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == RQLParser::MINUS) {
        setState(339);
        match(RQLParser::MINUS);
      }
      setState(342);
      match(RQLParser::FLOAT);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<ExpDecContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(344);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == RQLParser::MINUS) {
        setState(343);
        match(RQLParser::MINUS);
      }
      setState(346);
      match(RQLParser::DECIMAL);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<ExpConstContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(348);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == RQLParser::MINUS) {
        setState(347);
        match(RQLParser::MINUS);
      }
      setState(350);
      match(RQLParser::CONST_REF);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<ExpStringContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(351);
      match(RQLParser::STRING);
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<ExpUnaryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(352);
      unary_op_expression();
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<ExpFieldContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(353);
      field_id();
      break;
    }

    case 8: {
      _localctx = _tracker.createInstance<ExpWindowAggContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(354);
      window_agg();
      break;
    }

    case 9: {
      _localctx = _tracker.createInstance<ExpAggContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(355);
      agregator();
      break;
    }

    case 10: {
      _localctx = _tracker.createInstance<ExpGenIndexContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(356);
      match(RQLParser::DOLLAR);
      break;
    }

    case 11: {
      _localctx = _tracker.createInstance<ExpFnCallContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(357);
      function_call();
      break;
    }

    case 12: {
      _localctx = _tracker.createInstance<ExpNotContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(358);
      match(RQLParser::NOT_C);
      setState(359);
      term(1);
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(373);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 39, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(371);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 38, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpPowContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm);
          setState(362);

          if (!(precpred(_ctx, 15))) throw FailedPredicateException(this, "precpred(_ctx, 15)");
          setState(363);
          match(RQLParser::BIT_XOR);
          setState(364);
          term(15);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpMultContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm);
          setState(365);

          if (!(precpred(_ctx, 14))) throw FailedPredicateException(this, "precpred(_ctx, 14)");
          setState(366);
          match(RQLParser::STAR);
          setState(367);
          term(15);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<ExpDivContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm);
          setState(368);

          if (!(precpred(_ctx, 13))) throw FailedPredicateException(this, "precpred(_ctx, 13)");
          setState(369);
          match(RQLParser::DIVIDE);
          setState(370);
          term(14);
          break;
        }

        default:
          break;
        } 
      }
      setState(375);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 39, _ctx);
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

RQLParser::Int_constContext* RQLParser::SExpTimeMoveContext::int_const() {
  return getRuleContext<RQLParser::Int_constContext>(0);
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

std::vector<RQLParser::Int_constContext *> RQLParser::SExpAgseContext::int_const() {
  return getRuleContexts<RQLParser::Int_constContext>();
}

RQLParser::Int_constContext* RQLParser::SExpAgseContext::int_const(size_t i) {
  return getRuleContext<RQLParser::Int_constContext>(i);
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
  size_t startState = 56;
  enterRecursionRule(_localctx, 56, RQLParser::RuleStream_expression, precedence);

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

    setState(377);
    stream_factor();
    _ctx->stop = _input->LT(-1);
    setState(413);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 42, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(411);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 41, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<SExpHashContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(379);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(380);
          match(RQLParser::SHARP);
          setState(381);
          stream_expression(4);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<SExpPlusContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(382);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(383);
          match(RQLParser::PLUS);
          setState(384);
          stream_expression(3);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<SExpAgseContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(385);

          if (!(precpred(_ctx, 9))) throw FailedPredicateException(this, "precpred(_ctx, 9)");
          setState(386);
          match(RQLParser::AT);
          setState(387);
          match(RQLParser::T__2);
          setState(388);
          antlrcpp::downCast<SExpAgseContext *>(_localctx)->step = int_const();
          setState(389);
          match(RQLParser::COMMA);
          setState(391);
          _errHandler->sync(this);

          _la = _input->LA(1);
          if (_la == RQLParser::MINUS) {
            setState(390);
            match(RQLParser::MINUS);
          }
          setState(393);
          antlrcpp::downCast<SExpAgseContext *>(_localctx)->window = int_const();
          setState(394);
          match(RQLParser::T__3);
          break;
        }

        case 4: {
          auto newContext = _tracker.createInstance<SExpAndContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(396);

          if (!(precpred(_ctx, 8))) throw FailedPredicateException(this, "precpred(_ctx, 8)");
          setState(397);
          match(RQLParser::AND);
          setState(398);
          rational_se();
          break;
        }

        case 5: {
          auto newContext = _tracker.createInstance<SExpModContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(399);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(400);
          match(RQLParser::MOD);
          setState(401);
          rational_se();
          break;
        }

        case 6: {
          auto newContext = _tracker.createInstance<SExpTimeMoveContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(402);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(403);
          match(RQLParser::IS_GR);
          setState(404);
          int_const();
          break;
        }

        case 7: {
          auto newContext = _tracker.createInstance<SExpMinusContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(405);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(406);
          match(RQLParser::MINUS);
          setState(407);
          rational_se();
          break;
        }

        case 8: {
          auto newContext = _tracker.createInstance<SExpAgregate_proformaContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(408);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(409);
          match(RQLParser::DOT);
          setState(410);
          agregator();
          break;
        }

        default:
          break;
        } 
      }
      setState(415);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 42, _ctx);
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
  enterRule(_localctx, 58, RQLParser::RuleStream_factor);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(427);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 43, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(416);
      match(RQLParser::ID);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(417);
      match(RQLParser::ID);
      setState(418);
      match(RQLParser::T__0);
      setState(419);
      gen_index(0);
      setState(420);
      match(RQLParser::T__1);
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(422);
      match(RQLParser::T__2);
      setState(423);
      stream_expression(0);
      setState(424);
      match(RQLParser::T__3);
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(426);
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

tree::TerminalNode* RQLParser::Gen_indexContext::CONST_REF() {
  return getToken(RQLParser::CONST_REF, 0);
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
  size_t startState = 60;
  enterRecursionRule(_localctx, 60, RQLParser::RuleGen_index, precedence);

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
    setState(437);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::T__2: {
        setState(430);
        match(RQLParser::T__2);
        setState(431);
        gen_index(0);
        setState(432);
        match(RQLParser::T__3);
        break;
      }

      case RQLParser::DOLLAR: {
        setState(434);
        match(RQLParser::DOLLAR);
        break;
      }

      case RQLParser::DECIMAL: {
        setState(435);
        match(RQLParser::DECIMAL);
        break;
      }

      case RQLParser::CONST_REF: {
        setState(436);
        match(RQLParser::CONST_REF);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    _ctx->stop = _input->LT(-1);
    setState(447);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 46, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(445);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 45, _ctx)) {
        case 1: {
          _localctx = _tracker.createInstance<Gen_indexContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleGen_index);
          setState(439);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(440);
          match(RQLParser::STAR);
          setState(441);
          gen_index(7);
          break;
        }

        case 2: {
          _localctx = _tracker.createInstance<Gen_indexContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleGen_index);
          setState(442);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(443);
          _la = _input->LA(1);
          if (!(_la == RQLParser::PLUS

          || _la == RQLParser::MINUS)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(444);
          gen_index(6);
          break;
        }

        default:
          break;
        } 
      }
      setState(449);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 46, _ctx);
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
  enterRule(_localctx, 62, RQLParser::RuleAgregator);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(454);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::MIN: {
        _localctx = _tracker.createInstance<RQLParser::StreamMinContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(450);
        match(RQLParser::MIN);
        break;
      }

      case RQLParser::MAX: {
        _localctx = _tracker.createInstance<RQLParser::StreamMaxContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(451);
        match(RQLParser::MAX);
        break;
      }

      case RQLParser::AVG: {
        _localctx = _tracker.createInstance<RQLParser::StreamAvgContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(452);
        match(RQLParser::AVG);
        break;
      }

      case RQLParser::SUMC: {
        _localctx = _tracker.createInstance<RQLParser::StreamSumContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(453);
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
  enterRule(_localctx, 64, RQLParser::RuleStream_fn_call);
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
    setState(456);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 131941395333120) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(457);
    match(RQLParser::T__2);
    setState(458);
    stream_expression(0);
    setState(459);
    match(RQLParser::T__3);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Window_aggContext ------------------------------------------------------------------

RQLParser::Window_aggContext::Window_aggContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

RQLParser::Expression_factorContext* RQLParser::Window_aggContext::expression_factor() {
  return getRuleContext<RQLParser::Expression_factorContext>(0);
}

tree::TerminalNode* RQLParser::Window_aggContext::COLON() {
  return getToken(RQLParser::COLON, 0);
}

tree::TerminalNode* RQLParser::Window_aggContext::MIN() {
  return getToken(RQLParser::MIN, 0);
}

tree::TerminalNode* RQLParser::Window_aggContext::MAX() {
  return getToken(RQLParser::MAX, 0);
}

tree::TerminalNode* RQLParser::Window_aggContext::AVG() {
  return getToken(RQLParser::AVG, 0);
}

tree::TerminalNode* RQLParser::Window_aggContext::SUMC() {
  return getToken(RQLParser::SUMC, 0);
}

RQLParser::Int_constContext* RQLParser::Window_aggContext::int_const() {
  return getRuleContext<RQLParser::Int_constContext>(0);
}


size_t RQLParser::Window_aggContext::getRuleIndex() const {
  return RQLParser::RuleWindow_agg;
}

void RQLParser::Window_aggContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterWindow_agg(this);
}

void RQLParser::Window_aggContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitWindow_agg(this);
}

RQLParser::Window_aggContext* RQLParser::window_agg() {
  Window_aggContext *_localctx = _tracker.createInstance<Window_aggContext>(_ctx, getState());
  enterRule(_localctx, 66, RQLParser::RuleWindow_agg);
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
    setState(461);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 131941395333120) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(462);
    match(RQLParser::T__2);
    setState(463);
    expression_factor(0);
    setState(464);
    match(RQLParser::COLON);
    setState(465);
    antlrcpp::downCast<Window_aggContext *>(_localctx)->width = int_const();
    setState(466);
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

RQLParser::Expression_factorContext* RQLParser::Function_callContext::expression_factor() {
  return getRuleContext<RQLParser::Expression_factorContext>(0);
}

tree::TerminalNode* RQLParser::Function_callContext::ID() {
  return getToken(RQLParser::ID, 0);
}

tree::TerminalNode* RQLParser::Function_callContext::COLON() {
  return getToken(RQLParser::COLON, 0);
}

RQLParser::Int_constContext* RQLParser::Function_callContext::int_const() {
  return getRuleContext<RQLParser::Int_constContext>(0);
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
  enterRule(_localctx, 68, RQLParser::RuleFunction_call);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(480);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 48, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(468);
      antlrcpp::downCast<Function_callContext *>(_localctx)->fn = match(RQLParser::ID);
      setState(469);
      match(RQLParser::T__2);
      setState(470);
      expression_factor(0);
      setState(471);
      match(RQLParser::T__3);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(473);
      antlrcpp::downCast<Function_callContext *>(_localctx)->fn = match(RQLParser::ID);
      setState(474);
      match(RQLParser::T__2);
      setState(475);
      expression_factor(0);
      setState(476);
      match(RQLParser::COLON);
      setState(477);
      antlrcpp::downCast<Function_callContext *>(_localctx)->width = int_const();
      setState(478);
      match(RQLParser::T__3);
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

bool RQLParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 24: return expression_logicSempred(antlrcpp::downCast<Expression_logicContext *>(context), predicateIndex);
    case 25: return term_logicSempred(antlrcpp::downCast<Term_logicContext *>(context), predicateIndex);
    case 26: return expression_factorSempred(antlrcpp::downCast<Expression_factorContext *>(context), predicateIndex);
    case 27: return termSempred(antlrcpp::downCast<TermContext *>(context), predicateIndex);
    case 28: return stream_expressionSempred(antlrcpp::downCast<Stream_expressionContext *>(context), predicateIndex);
    case 30: return gen_indexSempred(antlrcpp::downCast<Gen_indexContext *>(context), predicateIndex);

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
    case 10: return precpred(_ctx, 15);
    case 11: return precpred(_ctx, 14);
    case 12: return precpred(_ctx, 13);

  default:
    break;
  }
  return true;
}

bool RQLParser::stream_expressionSempred(Stream_expressionContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 13: return precpred(_ctx, 3);
    case 14: return precpred(_ctx, 2);
    case 15: return precpred(_ctx, 9);
    case 16: return precpred(_ctx, 8);
    case 17: return precpred(_ctx, 7);
    case 18: return precpred(_ctx, 6);
    case 19: return precpred(_ctx, 5);
    case 20: return precpred(_ctx, 4);

  default:
    break;
  }
  return true;
}

bool RQLParser::gen_indexSempred(Gen_indexContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 21: return precpred(_ctx, 6);
    case 22: return precpred(_ctx, 5);

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
