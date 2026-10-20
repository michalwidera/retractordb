
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
      "var_type", "int_const", "compiler_option", "select_statement", "declare_statement", 
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
      "", "", "", "", "", "", "", "", "'='", "'!='", "'>'", "'<'", "'>='", 
      "'<='", "'!'", "'||'", "'.'", "'_'", "'@'", "'#'", "'&'", "'%'", "'$'", 
      "','", "';'", "':'", "'::'", "'*'", "'/'", "'+'", "'-'", "'~'", "'|'", 
      "'^'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "BYTE_T", "STRING_T", "UNSIGNED_T", "INTEGER_T", 
      "FLOAT_T", "DOUBLE_T", "RATIONAL_T", "SELECT", "STREAM", "FROM", "DECLARE", 
      "RETENTION", "FILE", "BINFILE", "TEXTFILE", "DEVICE", "STORAGE", "ROTATION", 
      "SUBSTRAT", "RULE", "DISPOSABLE", "ONESHOT", "HOLD", "TIMEOUT", "VOLATILE", 
      "PERSISTENT", "DEFAULT", "LET", "VAR", "ON", "WHEN", "DUMP", "SYSTEM", 
      "SET", "DO", "TO", "AND_C", "OR_C", "NOT_C", "MIN", "MAX", "AVG", 
      "SUMC", "TYPE_PROFILE", "STRING_PROFILE", "ID", "CONST_REF", "STRING", 
      "FLOAT", "DECIMAL", "REAL", "IS_EQ", "IS_NQ", "IS_GR", "IS_LS", "IS_GE", 
      "IS_LE", "EXCLAMATION", "DOUBLE_BAR", "DOT", "UNDERLINE", "AT", "SHARP", 
      "AND", "MOD", "DOLLAR", "COMMA", "SEMI", "COLON", "DOUBLE_COLON", 
      "STAR", "DIVIDE", "PLUS", "MINUS", "BIT_NOT", "BIT_OR", "BIT_XOR", 
      "SPACE", "COMMENT", "LINE_COMMENT2"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,84,491,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,7,
  	28,2,29,7,29,2,30,7,30,2,31,7,31,2,32,7,32,2,33,7,33,2,34,7,34,2,35,7,
  	35,1,0,1,0,1,0,1,0,1,0,1,0,1,0,4,0,80,8,0,11,0,12,0,81,1,0,1,0,1,1,1,
  	1,1,1,1,2,1,2,1,2,1,2,1,2,1,3,3,3,95,8,3,1,3,1,3,1,3,1,3,1,3,3,3,102,
  	8,3,1,4,1,4,1,4,1,4,3,4,108,8,4,1,4,1,4,1,4,1,5,1,5,1,6,1,6,1,7,1,7,1,
  	7,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,128,8,8,1,8,1,8,1,8,1,8,3,8,134,
  	8,8,1,8,3,8,137,8,8,1,8,3,8,140,8,8,1,8,1,8,3,8,144,8,8,1,9,1,9,1,9,1,
  	9,5,9,150,8,9,10,9,12,9,153,9,9,1,9,1,9,1,9,1,9,1,9,1,9,1,9,1,9,3,9,163,
  	8,9,1,9,3,9,166,8,9,1,9,3,9,169,8,9,1,9,3,9,172,8,9,1,9,3,9,175,8,9,1,
  	10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,1,10,3,10,187,8,10,1,11,1,
  	11,3,11,191,8,11,1,11,1,11,1,11,3,11,196,8,11,1,11,1,11,1,11,3,11,201,
  	8,11,1,12,1,12,1,12,1,13,1,13,1,13,1,13,1,13,1,14,1,14,1,14,1,14,3,14,
  	215,8,14,1,15,1,15,1,15,3,15,220,8,15,1,16,1,16,1,16,1,16,1,17,1,17,1,
  	17,1,17,1,17,1,17,3,17,232,8,17,1,18,1,18,1,18,1,18,1,18,1,18,3,18,240,
  	8,18,1,19,1,19,1,19,1,19,5,19,246,8,19,10,19,12,19,249,9,19,3,19,251,
  	8,19,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,1,20,
  	1,20,1,20,1,20,1,20,1,20,3,20,271,8,20,1,21,1,21,1,21,1,21,3,21,277,8,
  	21,1,22,1,22,3,22,281,8,22,1,22,1,22,1,23,1,23,1,24,1,24,1,25,1,25,1,
  	25,1,25,1,25,1,25,1,25,1,25,1,25,5,25,298,8,25,10,25,12,25,301,9,25,1,
  	26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,26,1,
  	26,1,26,1,26,1,26,1,26,1,26,1,26,5,26,324,8,26,10,26,12,26,327,9,26,1,
  	27,1,27,1,27,1,27,1,27,1,27,1,27,1,27,1,27,5,27,338,8,27,10,27,12,27,
  	341,9,27,1,28,1,28,1,28,1,28,1,28,1,28,3,28,349,8,28,1,28,1,28,3,28,353,
  	8,28,1,28,1,28,3,28,357,8,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,
  	1,28,1,28,3,28,369,8,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,
  	5,28,380,8,28,10,28,12,28,383,9,28,1,29,1,29,1,29,1,29,1,29,1,29,1,29,
  	1,29,1,29,1,29,1,29,1,29,1,29,1,29,1,29,3,29,400,8,29,1,29,1,29,1,29,
  	1,29,1,29,1,29,1,29,1,29,1,29,1,29,1,29,1,29,1,29,1,29,1,29,1,29,1,29,
  	1,29,5,29,420,8,29,10,29,12,29,423,9,29,1,30,1,30,1,30,1,30,1,30,1,30,
  	1,30,1,30,1,30,1,30,1,30,3,30,436,8,30,1,31,1,31,1,31,1,31,1,31,1,31,
  	1,31,1,31,3,31,446,8,31,1,31,1,31,1,31,1,31,1,31,1,31,5,31,454,8,31,10,
  	31,12,31,457,9,31,1,32,1,32,1,32,1,32,3,32,463,8,32,1,33,1,33,1,33,1,
  	33,1,33,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,35,1,35,1,35,1,35,1,35,1,
  	35,1,35,1,35,1,35,1,35,1,35,1,35,3,35,489,8,35,1,35,0,6,50,52,54,56,58,
  	62,36,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,
  	46,48,50,52,54,56,58,60,62,64,66,68,70,0,9,1,0,7,11,2,0,51,51,54,54,1,
  	0,21,23,2,0,49,49,52,52,1,0,29,30,4,0,18,20,31,31,48,48,50,50,1,0,17,
  	20,1,0,77,78,1,0,44,47,547,0,79,1,0,0,0,2,85,1,0,0,0,4,88,1,0,0,0,6,94,
  	1,0,0,0,8,103,1,0,0,0,10,112,1,0,0,0,12,114,1,0,0,0,14,116,1,0,0,0,16,
  	119,1,0,0,0,18,145,1,0,0,0,20,176,1,0,0,0,22,188,1,0,0,0,24,202,1,0,0,
  	0,26,205,1,0,0,0,28,214,1,0,0,0,30,216,1,0,0,0,32,221,1,0,0,0,34,225,
  	1,0,0,0,36,239,1,0,0,0,38,250,1,0,0,0,40,270,1,0,0,0,42,276,1,0,0,0,44,
  	280,1,0,0,0,46,284,1,0,0,0,48,286,1,0,0,0,50,288,1,0,0,0,52,302,1,0,0,
  	0,54,328,1,0,0,0,56,368,1,0,0,0,58,384,1,0,0,0,60,435,1,0,0,0,62,445,
  	1,0,0,0,64,462,1,0,0,0,66,464,1,0,0,0,68,469,1,0,0,0,70,488,1,0,0,0,72,
  	80,3,16,8,0,73,80,3,18,9,0,74,80,3,14,7,0,75,80,3,2,1,0,76,80,3,20,10,
  	0,77,80,3,4,2,0,78,80,3,8,4,0,79,72,1,0,0,0,79,73,1,0,0,0,79,74,1,0,0,
  	0,79,75,1,0,0,0,79,76,1,0,0,0,79,77,1,0,0,0,79,78,1,0,0,0,80,81,1,0,0,
  	0,81,79,1,0,0,0,81,82,1,0,0,0,82,83,1,0,0,0,83,84,5,0,0,1,84,1,1,0,0,
  	0,85,86,5,31,0,0,86,87,5,29,0,0,87,3,1,0,0,0,88,89,5,32,0,0,89,90,5,50,
  	0,0,90,91,5,56,0,0,91,92,3,6,3,0,92,5,1,0,0,0,93,95,5,78,0,0,94,93,1,
  	0,0,0,94,95,1,0,0,0,95,101,1,0,0,0,96,102,5,54,0,0,97,102,5,53,0,0,98,
  	99,5,54,0,0,99,100,5,76,0,0,100,102,5,54,0,0,101,96,1,0,0,0,101,97,1,
  	0,0,0,101,98,1,0,0,0,102,7,1,0,0,0,103,104,5,33,0,0,104,107,5,50,0,0,
  	105,106,5,73,0,0,106,108,3,10,5,0,107,105,1,0,0,0,107,108,1,0,0,0,108,
  	109,1,0,0,0,109,110,5,56,0,0,110,111,3,6,3,0,111,9,1,0,0,0,112,113,7,
  	0,0,0,113,11,1,0,0,0,114,115,7,1,0,0,115,13,1,0,0,0,116,117,7,2,0,0,117,
  	118,7,3,0,0,118,15,1,0,0,0,119,120,5,12,0,0,120,121,3,38,19,0,121,122,
  	5,13,0,0,122,127,5,50,0,0,123,124,5,1,0,0,124,125,3,12,6,0,125,126,5,
  	2,0,0,126,128,1,0,0,0,127,123,1,0,0,0,127,128,1,0,0,0,128,129,1,0,0,0,
  	129,130,5,14,0,0,130,133,3,58,29,0,131,132,5,17,0,0,132,134,5,52,0,0,
  	133,131,1,0,0,0,133,134,1,0,0,0,134,136,1,0,0,0,135,137,3,30,15,0,136,
  	135,1,0,0,0,136,137,1,0,0,0,137,139,1,0,0,0,138,140,7,4,0,0,139,138,1,
  	0,0,0,139,140,1,0,0,0,140,143,1,0,0,0,141,142,5,21,0,0,142,144,7,5,0,
  	0,143,141,1,0,0,0,143,144,1,0,0,0,144,17,1,0,0,0,145,146,5,15,0,0,146,
  	151,3,34,17,0,147,148,5,71,0,0,148,150,3,34,17,0,149,147,1,0,0,0,150,
  	153,1,0,0,0,151,149,1,0,0,0,151,152,1,0,0,0,152,154,1,0,0,0,153,151,1,
  	0,0,0,154,155,5,13,0,0,155,156,5,50,0,0,156,157,5,71,0,0,157,158,3,28,
  	14,0,158,159,7,6,0,0,159,165,5,52,0,0,160,162,5,28,0,0,161,163,5,78,0,
  	0,162,161,1,0,0,0,162,163,1,0,0,0,163,164,1,0,0,0,164,166,3,28,14,0,165,
  	160,1,0,0,0,165,166,1,0,0,0,166,168,1,0,0,0,167,169,5,25,0,0,168,167,
  	1,0,0,0,168,169,1,0,0,0,169,171,1,0,0,0,170,172,5,26,0,0,171,170,1,0,
  	0,0,171,172,1,0,0,0,172,174,1,0,0,0,173,175,5,27,0,0,174,173,1,0,0,0,
  	174,175,1,0,0,0,175,19,1,0,0,0,176,177,5,24,0,0,177,178,5,50,0,0,178,
  	179,5,34,0,0,179,180,5,50,0,0,180,181,5,35,0,0,181,182,3,48,24,0,182,
  	186,5,39,0,0,183,187,3,22,11,0,184,187,3,24,12,0,185,187,3,26,13,0,186,
  	183,1,0,0,0,186,184,1,0,0,0,186,185,1,0,0,0,187,21,1,0,0,0,188,190,5,
  	36,0,0,189,191,5,78,0,0,190,189,1,0,0,0,190,191,1,0,0,0,191,192,1,0,0,
  	0,192,193,3,12,6,0,193,195,5,40,0,0,194,196,5,78,0,0,195,194,1,0,0,0,
  	195,196,1,0,0,0,196,197,1,0,0,0,197,200,3,12,6,0,198,199,5,16,0,0,199,
  	201,3,12,6,0,200,198,1,0,0,0,200,201,1,0,0,0,201,23,1,0,0,0,202,203,5,
  	37,0,0,203,204,5,52,0,0,204,25,1,0,0,0,205,206,5,38,0,0,206,207,5,51,
  	0,0,207,208,5,56,0,0,208,209,3,54,27,0,209,27,1,0,0,0,210,215,3,32,16,
  	0,211,215,5,53,0,0,212,215,5,54,0,0,213,215,5,51,0,0,214,210,1,0,0,0,
  	214,211,1,0,0,0,214,212,1,0,0,0,214,213,1,0,0,0,215,29,1,0,0,0,216,217,
  	5,16,0,0,217,219,3,12,6,0,218,220,3,12,6,0,219,218,1,0,0,0,219,220,1,
  	0,0,0,220,31,1,0,0,0,221,222,5,54,0,0,222,223,5,76,0,0,223,224,5,54,0,
  	0,224,33,1,0,0,0,225,226,5,50,0,0,226,231,3,36,18,0,227,228,5,1,0,0,228,
  	229,3,12,6,0,229,230,5,2,0,0,230,232,1,0,0,0,231,227,1,0,0,0,231,232,
  	1,0,0,0,232,35,1,0,0,0,233,240,5,5,0,0,234,240,5,8,0,0,235,240,5,7,0,
  	0,236,240,5,9,0,0,237,240,5,10,0,0,238,240,5,6,0,0,239,233,1,0,0,0,239,
  	234,1,0,0,0,239,235,1,0,0,0,239,236,1,0,0,0,239,237,1,0,0,0,239,238,1,
  	0,0,0,240,37,1,0,0,0,241,251,3,44,22,0,242,247,3,46,23,0,243,244,5,71,
  	0,0,244,246,3,46,23,0,245,243,1,0,0,0,246,249,1,0,0,0,247,245,1,0,0,0,
  	247,248,1,0,0,0,248,251,1,0,0,0,249,247,1,0,0,0,250,241,1,0,0,0,250,242,
  	1,0,0,0,251,39,1,0,0,0,252,271,5,50,0,0,253,254,5,50,0,0,254,255,5,1,
  	0,0,255,256,5,65,0,0,256,271,5,2,0,0,257,258,5,50,0,0,258,259,5,64,0,
  	0,259,271,5,50,0,0,260,261,5,50,0,0,261,262,5,1,0,0,262,263,3,12,6,0,
  	263,264,5,2,0,0,264,271,1,0,0,0,265,266,5,50,0,0,266,267,5,1,0,0,267,
  	268,3,62,31,0,268,269,5,2,0,0,269,271,1,0,0,0,270,252,1,0,0,0,270,253,
  	1,0,0,0,270,257,1,0,0,0,270,260,1,0,0,0,270,265,1,0,0,0,271,41,1,0,0,
  	0,272,273,5,79,0,0,273,277,3,56,28,0,274,275,7,7,0,0,275,277,3,56,28,
  	0,276,272,1,0,0,0,276,274,1,0,0,0,277,43,1,0,0,0,278,279,5,50,0,0,279,
  	281,5,64,0,0,280,278,1,0,0,0,280,281,1,0,0,0,281,282,1,0,0,0,282,283,
  	5,75,0,0,283,45,1,0,0,0,284,285,3,54,27,0,285,47,1,0,0,0,286,287,3,50,
  	25,0,287,49,1,0,0,0,288,289,6,25,-1,0,289,290,3,52,26,0,290,299,1,0,0,
  	0,291,292,10,3,0,0,292,293,5,41,0,0,293,298,3,50,25,4,294,295,10,2,0,
  	0,295,296,5,42,0,0,296,298,3,50,25,3,297,291,1,0,0,0,297,294,1,0,0,0,
  	298,301,1,0,0,0,299,297,1,0,0,0,299,300,1,0,0,0,300,51,1,0,0,0,301,299,
  	1,0,0,0,302,303,6,26,-1,0,303,304,3,54,27,0,304,325,1,0,0,0,305,306,10,
  	7,0,0,306,307,5,56,0,0,307,324,3,52,26,8,308,309,10,6,0,0,309,310,5,57,
  	0,0,310,324,3,52,26,7,311,312,10,5,0,0,312,313,5,58,0,0,313,324,3,52,
  	26,6,314,315,10,4,0,0,315,316,5,59,0,0,316,324,3,52,26,5,317,318,10,3,
  	0,0,318,319,5,60,0,0,319,324,3,52,26,4,320,321,10,2,0,0,321,322,5,61,
  	0,0,322,324,3,52,26,3,323,305,1,0,0,0,323,308,1,0,0,0,323,311,1,0,0,0,
  	323,314,1,0,0,0,323,317,1,0,0,0,323,320,1,0,0,0,324,327,1,0,0,0,325,323,
  	1,0,0,0,325,326,1,0,0,0,326,53,1,0,0,0,327,325,1,0,0,0,328,329,6,27,-1,
  	0,329,330,3,56,28,0,330,339,1,0,0,0,331,332,10,3,0,0,332,333,5,77,0,0,
  	333,338,3,54,27,4,334,335,10,2,0,0,335,336,5,78,0,0,336,338,3,54,27,3,
  	337,331,1,0,0,0,337,334,1,0,0,0,338,341,1,0,0,0,339,337,1,0,0,0,339,340,
  	1,0,0,0,340,55,1,0,0,0,341,339,1,0,0,0,342,343,6,28,-1,0,343,344,5,3,
  	0,0,344,345,3,54,27,0,345,346,5,4,0,0,346,369,1,0,0,0,347,349,5,78,0,
  	0,348,347,1,0,0,0,348,349,1,0,0,0,349,350,1,0,0,0,350,369,5,53,0,0,351,
  	353,5,78,0,0,352,351,1,0,0,0,352,353,1,0,0,0,353,354,1,0,0,0,354,369,
  	5,54,0,0,355,357,5,78,0,0,356,355,1,0,0,0,356,357,1,0,0,0,357,358,1,0,
  	0,0,358,369,5,51,0,0,359,369,5,52,0,0,360,369,3,42,21,0,361,369,3,40,
  	20,0,362,369,3,68,34,0,363,369,3,64,32,0,364,369,5,70,0,0,365,369,3,70,
  	35,0,366,367,5,43,0,0,367,369,3,56,28,1,368,342,1,0,0,0,368,348,1,0,0,
  	0,368,352,1,0,0,0,368,356,1,0,0,0,368,359,1,0,0,0,368,360,1,0,0,0,368,
  	361,1,0,0,0,368,362,1,0,0,0,368,363,1,0,0,0,368,364,1,0,0,0,368,365,1,
  	0,0,0,368,366,1,0,0,0,369,381,1,0,0,0,370,371,10,15,0,0,371,372,5,81,
  	0,0,372,380,3,56,28,15,373,374,10,14,0,0,374,375,5,75,0,0,375,380,3,56,
  	28,15,376,377,10,13,0,0,377,378,5,76,0,0,378,380,3,56,28,14,379,370,1,
  	0,0,0,379,373,1,0,0,0,379,376,1,0,0,0,380,383,1,0,0,0,381,379,1,0,0,0,
  	381,382,1,0,0,0,382,57,1,0,0,0,383,381,1,0,0,0,384,385,6,29,-1,0,385,
  	386,3,60,30,0,386,421,1,0,0,0,387,388,10,3,0,0,388,389,5,67,0,0,389,420,
  	3,58,29,4,390,391,10,2,0,0,391,392,5,77,0,0,392,420,3,58,29,3,393,394,
  	10,9,0,0,394,395,5,66,0,0,395,396,5,3,0,0,396,397,3,12,6,0,397,399,5,
  	71,0,0,398,400,5,78,0,0,399,398,1,0,0,0,399,400,1,0,0,0,400,401,1,0,0,
  	0,401,402,3,12,6,0,402,403,5,4,0,0,403,420,1,0,0,0,404,405,10,8,0,0,405,
  	406,5,68,0,0,406,420,3,28,14,0,407,408,10,7,0,0,408,409,5,69,0,0,409,
  	420,3,28,14,0,410,411,10,6,0,0,411,412,5,58,0,0,412,420,3,12,6,0,413,
  	414,10,5,0,0,414,415,5,78,0,0,415,420,3,28,14,0,416,417,10,4,0,0,417,
  	418,5,64,0,0,418,420,3,64,32,0,419,387,1,0,0,0,419,390,1,0,0,0,419,393,
  	1,0,0,0,419,404,1,0,0,0,419,407,1,0,0,0,419,410,1,0,0,0,419,413,1,0,0,
  	0,419,416,1,0,0,0,420,423,1,0,0,0,421,419,1,0,0,0,421,422,1,0,0,0,422,
  	59,1,0,0,0,423,421,1,0,0,0,424,436,5,50,0,0,425,426,5,50,0,0,426,427,
  	5,1,0,0,427,428,3,62,31,0,428,429,5,2,0,0,429,436,1,0,0,0,430,431,5,3,
  	0,0,431,432,3,58,29,0,432,433,5,4,0,0,433,436,1,0,0,0,434,436,3,66,33,
  	0,435,424,1,0,0,0,435,425,1,0,0,0,435,430,1,0,0,0,435,434,1,0,0,0,436,
  	61,1,0,0,0,437,438,6,31,-1,0,438,439,5,3,0,0,439,440,3,62,31,0,440,441,
  	5,4,0,0,441,446,1,0,0,0,442,446,5,70,0,0,443,446,5,54,0,0,444,446,5,51,
  	0,0,445,437,1,0,0,0,445,442,1,0,0,0,445,443,1,0,0,0,445,444,1,0,0,0,446,
  	455,1,0,0,0,447,448,10,6,0,0,448,449,5,75,0,0,449,454,3,62,31,7,450,451,
  	10,5,0,0,451,452,7,7,0,0,452,454,3,62,31,6,453,447,1,0,0,0,453,450,1,
  	0,0,0,454,457,1,0,0,0,455,453,1,0,0,0,455,456,1,0,0,0,456,63,1,0,0,0,
  	457,455,1,0,0,0,458,463,5,44,0,0,459,463,5,45,0,0,460,463,5,46,0,0,461,
  	463,5,47,0,0,462,458,1,0,0,0,462,459,1,0,0,0,462,460,1,0,0,0,462,461,
  	1,0,0,0,463,65,1,0,0,0,464,465,7,8,0,0,465,466,5,3,0,0,466,467,3,58,29,
  	0,467,468,5,4,0,0,468,67,1,0,0,0,469,470,7,8,0,0,470,471,5,3,0,0,471,
  	472,3,54,27,0,472,473,5,73,0,0,473,474,3,12,6,0,474,475,5,4,0,0,475,69,
  	1,0,0,0,476,477,5,50,0,0,477,478,5,3,0,0,478,479,3,54,27,0,479,480,5,
  	4,0,0,480,489,1,0,0,0,481,482,5,50,0,0,482,483,5,3,0,0,483,484,3,54,27,
  	0,484,485,5,73,0,0,485,486,3,12,6,0,486,487,5,4,0,0,487,489,1,0,0,0,488,
  	476,1,0,0,0,488,481,1,0,0,0,489,71,1,0,0,0,50,79,81,94,101,107,127,133,
  	136,139,143,151,162,165,168,171,174,186,190,195,200,214,219,231,239,247,
  	250,270,276,280,297,299,323,325,337,339,348,352,356,368,379,381,399,419,
  	421,435,445,453,455,462,488
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
    setState(79); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(79);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case RQLParser::SELECT: {
          setState(72);
          select_statement();
          break;
        }

        case RQLParser::DECLARE: {
          setState(73);
          declare_statement();
          break;
        }

        case RQLParser::STORAGE:
        case RQLParser::ROTATION:
        case RQLParser::SUBSTRAT: {
          setState(74);
          compiler_option();
          break;
        }

        case RQLParser::DEFAULT: {
          setState(75);
          default_statement();
          break;
        }

        case RQLParser::RULE: {
          setState(76);
          rule_statement();
          break;
        }

        case RQLParser::LET: {
          setState(77);
          let_statement();
          break;
        }

        case RQLParser::VAR: {
          setState(78);
          var_statement();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(81); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 15063879680) != 0));
    setState(83);
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
    setState(85);
    match(RQLParser::DEFAULT);
    setState(86);
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
    setState(88);
    match(RQLParser::LET);
    setState(89);
    antlrcpp::downCast<LetContext *>(_localctx)->name = match(RQLParser::ID);
    setState(90);
    match(RQLParser::IS_EQ);
    setState(91);
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
    setState(94);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::MINUS) {
      setState(93);
      antlrcpp::downCast<Let_valueContext *>(_localctx)->sign = match(RQLParser::MINUS);
    }
    setState(101);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 3, _ctx)) {
    case 1: {
      setState(96);
      match(RQLParser::DECIMAL);
      break;
    }

    case 2: {
      setState(97);
      match(RQLParser::FLOAT);
      break;
    }

    case 3: {
      setState(98);
      antlrcpp::downCast<Let_valueContext *>(_localctx)->nom = match(RQLParser::DECIMAL);
      setState(99);
      match(RQLParser::DIVIDE);
      setState(100);
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

tree::TerminalNode* RQLParser::VarContext::COLON() {
  return getToken(RQLParser::COLON, 0);
}

RQLParser::Var_typeContext* RQLParser::VarContext::var_type() {
  return getRuleContext<RQLParser::Var_typeContext>(0);
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
  size_t _la = 0;

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
    setState(103);
    match(RQLParser::VAR);
    setState(104);
    antlrcpp::downCast<VarContext *>(_localctx)->name = match(RQLParser::ID);
    setState(107);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::COLON) {
      setState(105);
      match(RQLParser::COLON);
      setState(106);
      var_type();
    }
    setState(109);
    match(RQLParser::IS_EQ);
    setState(110);
    let_value();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Var_typeContext ------------------------------------------------------------------

RQLParser::Var_typeContext::Var_typeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* RQLParser::Var_typeContext::INTEGER_T() {
  return getToken(RQLParser::INTEGER_T, 0);
}

tree::TerminalNode* RQLParser::Var_typeContext::UNSIGNED_T() {
  return getToken(RQLParser::UNSIGNED_T, 0);
}

tree::TerminalNode* RQLParser::Var_typeContext::FLOAT_T() {
  return getToken(RQLParser::FLOAT_T, 0);
}

tree::TerminalNode* RQLParser::Var_typeContext::DOUBLE_T() {
  return getToken(RQLParser::DOUBLE_T, 0);
}

tree::TerminalNode* RQLParser::Var_typeContext::RATIONAL_T() {
  return getToken(RQLParser::RATIONAL_T, 0);
}


size_t RQLParser::Var_typeContext::getRuleIndex() const {
  return RQLParser::RuleVar_type;
}

void RQLParser::Var_typeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterVar_type(this);
}

void RQLParser::Var_typeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<RQLListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitVar_type(this);
}

RQLParser::Var_typeContext* RQLParser::var_type() {
  Var_typeContext *_localctx = _tracker.createInstance<Var_typeContext>(_ctx, getState());
  enterRule(_localctx, 10, RQLParser::RuleVar_type);
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
    setState(112);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 3968) != 0))) {
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
  enterRule(_localctx, 12, RQLParser::RuleInt_const);
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
    setState(114);
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
  enterRule(_localctx, 14, RQLParser::RuleCompiler_option);
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
    setState(116);
    antlrcpp::downCast<CoptionContext *>(_localctx)->directive = _input->LT(1);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 14680064) != 0))) {
      antlrcpp::downCast<CoptionContext *>(_localctx)->directive = _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(117);
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
  enterRule(_localctx, 16, RQLParser::RuleSelect_statement);
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
    setState(119);
    match(RQLParser::SELECT);
    setState(120);
    select_list();
    setState(121);
    match(RQLParser::STREAM);
    setState(122);
    antlrcpp::downCast<SelectContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(127);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::T__0) {
      setState(123);
      match(RQLParser::T__0);
      setState(124);
      antlrcpp::downCast<SelectContext *>(_localctx)->gen_size = int_const();
      setState(125);
      match(RQLParser::T__1);
    }
    setState(129);
    match(RQLParser::FROM);
    setState(130);
    stream_expression(0);
    setState(133);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::FILE) {
      setState(131);
      match(RQLParser::FILE);
      setState(132);
      antlrcpp::downCast<SelectContext *>(_localctx)->file_name = match(RQLParser::STRING);
    }
    setState(136);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::RETENTION) {
      setState(135);
      retention_from();
    }
    setState(139);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::VOLATILE

    || _la == RQLParser::PERSISTENT) {
      setState(138);
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
    setState(143);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx)) {
    case 1: {
      setState(141);
      match(RQLParser::STORAGE);
      setState(142);
      antlrcpp::downCast<SelectContext *>(_localctx)->type_name = _input->LT(1);
      _la = _input->LA(1);
      if (!((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 1407377032871936) != 0))) {
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
  enterRule(_localctx, 18, RQLParser::RuleDeclare_statement);
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
    setState(145);
    match(RQLParser::DECLARE);
    setState(146);
    field_declaration();
    setState(151);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == RQLParser::COMMA) {
      setState(147);
      match(RQLParser::COMMA);
      setState(148);
      field_declaration();
      setState(153);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(154);
    match(RQLParser::STREAM);
    setState(155);
    antlrcpp::downCast<DeclareContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(156);
    match(RQLParser::COMMA);
    setState(157);
    rational_se();
    setState(158);
    antlrcpp::downCast<DeclareContext *>(_localctx)->kind = _input->LT(1);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1966080) != 0))) {
      antlrcpp::downCast<DeclareContext *>(_localctx)->kind = _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(159);
    antlrcpp::downCast<DeclareContext *>(_localctx)->file_name = match(RQLParser::STRING);
    setState(165);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::TIMEOUT) {
      setState(160);
      match(RQLParser::TIMEOUT);
      setState(162);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == RQLParser::MINUS) {
        setState(161);
        antlrcpp::downCast<DeclareContext *>(_localctx)->timeout_sign = match(RQLParser::MINUS);
      }
      setState(164);
      antlrcpp::downCast<DeclareContext *>(_localctx)->timeout_value = rational_se();
    }
    setState(168);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::DISPOSABLE) {
      setState(167);
      match(RQLParser::DISPOSABLE);
    }
    setState(171);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::ONESHOT) {
      setState(170);
      match(RQLParser::ONESHOT);
    }
    setState(174);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::HOLD) {
      setState(173);
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
  enterRule(_localctx, 20, RQLParser::RuleRule_statement);

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
    setState(176);
    match(RQLParser::RULE);
    setState(177);
    antlrcpp::downCast<RulezContext *>(_localctx)->name = match(RQLParser::ID);
    setState(178);
    match(RQLParser::ON);
    setState(179);
    antlrcpp::downCast<RulezContext *>(_localctx)->stream_name = match(RQLParser::ID);
    setState(180);
    match(RQLParser::WHEN);
    setState(181);
    logic();
    setState(182);
    match(RQLParser::DO);
    setState(186);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::DUMP: {
        setState(183);
        dumppart();
        break;
      }

      case RQLParser::SYSTEM: {
        setState(184);
        systempart();
        break;
      }

      case RQLParser::SET: {
        setState(185);
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
  enterRule(_localctx, 22, RQLParser::RuleDumppart);
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
    setState(188);
    match(RQLParser::DUMP);
    setState(190);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::MINUS) {
      setState(189);
      match(RQLParser::MINUS);
    }
    setState(192);
    antlrcpp::downCast<DumppartContext *>(_localctx)->step_back = int_const();
    setState(193);
    match(RQLParser::TO);
    setState(195);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::MINUS) {
      setState(194);
      match(RQLParser::MINUS);
    }
    setState(197);
    antlrcpp::downCast<DumppartContext *>(_localctx)->step_forward = int_const();
    setState(200);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::RETENTION) {
      setState(198);
      match(RQLParser::RETENTION);
      setState(199);
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
  enterRule(_localctx, 24, RQLParser::RuleSystempart);

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
    match(RQLParser::SYSTEM);
    setState(203);
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
  enterRule(_localctx, 26, RQLParser::RuleSetpart);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(205);
    match(RQLParser::SET);
    setState(206);
    antlrcpp::downCast<SetpartContext *>(_localctx)->target = match(RQLParser::CONST_REF);
    setState(207);
    match(RQLParser::IS_EQ);
    setState(208);
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
  enterRule(_localctx, 28, RQLParser::RuleRational_se);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(214);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 20, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsFraction_proformaContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(210);
      fraction_rule();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsFloatContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(211);
      match(RQLParser::FLOAT);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsDecimalContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(212);
      match(RQLParser::DECIMAL);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<RQLParser::RationalAsConstContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(213);
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
  enterRule(_localctx, 30, RQLParser::RuleRetention_from);
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
    setState(216);
    match(RQLParser::RETENTION);
    setState(217);
    antlrcpp::downCast<RetentionContext *>(_localctx)->capacity = int_const();
    setState(219);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::CONST_REF

    || _la == RQLParser::DECIMAL) {
      setState(218);
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
  enterRule(_localctx, 32, RQLParser::RuleFraction_rule);

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
    setState(221);
    match(RQLParser::DECIMAL);
    setState(222);
    match(RQLParser::DIVIDE);
    setState(223);
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
  enterRule(_localctx, 34, RQLParser::RuleField_declaration);
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
    setState(225);
    match(RQLParser::ID);
    setState(226);
    field_type();
    setState(231);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::T__0) {
      setState(227);
      match(RQLParser::T__0);
      setState(228);
      antlrcpp::downCast<SingleDeclarationContext *>(_localctx)->type_size = int_const();
      setState(229);
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
  enterRule(_localctx, 36, RQLParser::RuleField_type);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(239);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::BYTE_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeByteContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(233);
        match(RQLParser::BYTE_T);
        break;
      }

      case RQLParser::INTEGER_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeIntContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(234);
        match(RQLParser::INTEGER_T);
        break;
      }

      case RQLParser::UNSIGNED_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeUnsignedContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(235);
        match(RQLParser::UNSIGNED_T);
        break;
      }

      case RQLParser::FLOAT_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeFloatContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(236);
        match(RQLParser::FLOAT_T);
        break;
      }

      case RQLParser::DOUBLE_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeDoubleContext>(_localctx);
        enterOuterAlt(_localctx, 5);
        setState(237);
        match(RQLParser::DOUBLE_T);
        break;
      }

      case RQLParser::STRING_T: {
        _localctx = _tracker.createInstance<RQLParser::TypeStringContext>(_localctx);
        enterOuterAlt(_localctx, 6);
        setState(238);
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
  enterRule(_localctx, 38, RQLParser::RuleSelect_list);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(250);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 25, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::SelectListFullscanContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(241);
      asterisk();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::SelectListContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(242);
      expression();
      setState(247);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == RQLParser::COMMA) {
        setState(243);
        match(RQLParser::COMMA);
        setState(244);
        expression();
        setState(249);
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
  enterRule(_localctx, 40, RQLParser::RuleField_id);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(270);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 26, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(252);
      antlrcpp::downCast<FieldIDContext *>(_localctx)->column_name = match(RQLParser::ID);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDUnderlineContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(253);
      antlrcpp::downCast<FieldIDUnderlineContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(254);
      match(RQLParser::T__0);
      setState(255);
      match(RQLParser::UNDERLINE);
      setState(256);
      match(RQLParser::T__1);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDColumnNameContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(257);
      antlrcpp::downCast<FieldIDColumnNameContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(258);
      match(RQLParser::DOT);
      setState(259);
      antlrcpp::downCast<FieldIDColumnNameContext *>(_localctx)->column_name = match(RQLParser::ID);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDTableContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(260);
      antlrcpp::downCast<FieldIDTableContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(261);
      match(RQLParser::T__0);
      setState(262);
      antlrcpp::downCast<FieldIDTableContext *>(_localctx)->column_index = int_const();
      setState(263);
      match(RQLParser::T__1);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<RQLParser::FieldIDGeneratedContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(265);
      antlrcpp::downCast<FieldIDGeneratedContext *>(_localctx)->tablename = match(RQLParser::ID);
      setState(266);
      match(RQLParser::T__0);
      setState(267);
      gen_index(0);
      setState(268);
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
  enterRule(_localctx, 42, RQLParser::RuleUnary_op_expression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(276);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::BIT_NOT: {
        enterOuterAlt(_localctx, 1);
        setState(272);
        match(RQLParser::BIT_NOT);
        setState(273);
        term(0);
        break;
      }

      case RQLParser::PLUS:
      case RQLParser::MINUS: {
        enterOuterAlt(_localctx, 2);
        setState(274);
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
        setState(275);
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
  enterRule(_localctx, 44, RQLParser::RuleAsterisk);
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
    setState(280);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == RQLParser::ID) {
      setState(278);
      match(RQLParser::ID);
      setState(279);
      match(RQLParser::DOT);
    }
    setState(282);
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
  enterRule(_localctx, 46, RQLParser::RuleExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(284);
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
  enterRule(_localctx, 48, RQLParser::RuleLogic);

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
    setState(286);
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
  size_t startState = 50;
  enterRecursionRule(_localctx, 50, RQLParser::RuleExpression_logic, precedence);

    

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

    setState(289);
    term_logic(0);
    _ctx->stop = _input->LT(-1);
    setState(299);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 30, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(297);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpAndContext>(_tracker.createInstance<Expression_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_logic);
          setState(291);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(292);
          match(RQLParser::AND_C);
          setState(293);
          expression_logic(4);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpOrContext>(_tracker.createInstance<Expression_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_logic);
          setState(294);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(295);
          match(RQLParser::OR_C);
          setState(296);
          expression_logic(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(301);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 30, _ctx);
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
  size_t startState = 52;
  enterRecursionRule(_localctx, 52, RQLParser::RuleTerm_logic, precedence);

    

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

    setState(303);
    expression_factor(0);
    _ctx->stop = _input->LT(-1);
    setState(325);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 32, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(323);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 31, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpEqContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(305);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(306);
          match(RQLParser::IS_EQ);
          setState(307);
          term_logic(8);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpNqContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(308);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(309);
          match(RQLParser::IS_NQ);
          setState(310);
          term_logic(7);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<ExpGrContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(311);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(312);
          match(RQLParser::IS_GR);
          setState(313);
          term_logic(6);
          break;
        }

        case 4: {
          auto newContext = _tracker.createInstance<ExpLsContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(314);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(315);
          match(RQLParser::IS_LS);
          setState(316);
          term_logic(5);
          break;
        }

        case 5: {
          auto newContext = _tracker.createInstance<ExpGeContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(317);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(318);
          match(RQLParser::IS_GE);
          setState(319);
          term_logic(4);
          break;
        }

        case 6: {
          auto newContext = _tracker.createInstance<ExpLeContext>(_tracker.createInstance<Term_logicContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm_logic);
          setState(320);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(321);
          match(RQLParser::IS_LE);
          setState(322);
          term_logic(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(327);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 32, _ctx);
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
  size_t startState = 54;
  enterRecursionRule(_localctx, 54, RQLParser::RuleExpression_factor, precedence);

    

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

    setState(329);
    term(0);
    _ctx->stop = _input->LT(-1);
    setState(339);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 34, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(337);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 33, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpPlusContext>(_tracker.createInstance<Expression_factorContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_factor);
          setState(331);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(332);
          match(RQLParser::PLUS);
          setState(333);
          expression_factor(4);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpMinusContext>(_tracker.createInstance<Expression_factorContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression_factor);
          setState(334);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(335);
          match(RQLParser::MINUS);
          setState(336);
          expression_factor(3);
          break;
        }

        default:
          break;
        } 
      }
      setState(341);
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
  size_t startState = 56;
  enterRecursionRule(_localctx, 56, RQLParser::RuleTerm, precedence);

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
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 38, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<ExpInContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(343);
      match(RQLParser::T__2);
      setState(344);
      expression_factor(0);
      setState(345);
      match(RQLParser::T__3);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<ExpFloatContext>(_localctx);
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
      match(RQLParser::FLOAT);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<ExpDecContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(352);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == RQLParser::MINUS) {
        setState(351);
        match(RQLParser::MINUS);
      }
      setState(354);
      match(RQLParser::DECIMAL);
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<ExpConstContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(356);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == RQLParser::MINUS) {
        setState(355);
        match(RQLParser::MINUS);
      }
      setState(358);
      match(RQLParser::CONST_REF);
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<ExpStringContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(359);
      match(RQLParser::STRING);
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<ExpUnaryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(360);
      unary_op_expression();
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<ExpFieldContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(361);
      field_id();
      break;
    }

    case 8: {
      _localctx = _tracker.createInstance<ExpWindowAggContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(362);
      window_agg();
      break;
    }

    case 9: {
      _localctx = _tracker.createInstance<ExpAggContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(363);
      agregator();
      break;
    }

    case 10: {
      _localctx = _tracker.createInstance<ExpGenIndexContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(364);
      match(RQLParser::DOLLAR);
      break;
    }

    case 11: {
      _localctx = _tracker.createInstance<ExpFnCallContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(365);
      function_call();
      break;
    }

    case 12: {
      _localctx = _tracker.createInstance<ExpNotContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(366);
      match(RQLParser::NOT_C);
      setState(367);
      term(1);
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(381);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 40, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(379);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 39, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<ExpPowContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm);
          setState(370);

          if (!(precpred(_ctx, 15))) throw FailedPredicateException(this, "precpred(_ctx, 15)");
          setState(371);
          match(RQLParser::BIT_XOR);
          setState(372);
          term(15);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<ExpMultContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm);
          setState(373);

          if (!(precpred(_ctx, 14))) throw FailedPredicateException(this, "precpred(_ctx, 14)");
          setState(374);
          match(RQLParser::STAR);
          setState(375);
          term(15);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<ExpDivContext>(_tracker.createInstance<TermContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleTerm);
          setState(376);

          if (!(precpred(_ctx, 13))) throw FailedPredicateException(this, "precpred(_ctx, 13)");
          setState(377);
          match(RQLParser::DIVIDE);
          setState(378);
          term(14);
          break;
        }

        default:
          break;
        } 
      }
      setState(383);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 40, _ctx);
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
  size_t startState = 58;
  enterRecursionRule(_localctx, 58, RQLParser::RuleStream_expression, precedence);

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

    setState(385);
    stream_factor();
    _ctx->stop = _input->LT(-1);
    setState(421);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 43, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(419);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 42, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<SExpHashContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(387);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(388);
          match(RQLParser::SHARP);
          setState(389);
          stream_expression(4);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<SExpPlusContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(390);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(391);
          match(RQLParser::PLUS);
          setState(392);
          stream_expression(3);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<SExpAgseContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(393);

          if (!(precpred(_ctx, 9))) throw FailedPredicateException(this, "precpred(_ctx, 9)");
          setState(394);
          match(RQLParser::AT);
          setState(395);
          match(RQLParser::T__2);
          setState(396);
          antlrcpp::downCast<SExpAgseContext *>(_localctx)->step = int_const();
          setState(397);
          match(RQLParser::COMMA);
          setState(399);
          _errHandler->sync(this);

          _la = _input->LA(1);
          if (_la == RQLParser::MINUS) {
            setState(398);
            match(RQLParser::MINUS);
          }
          setState(401);
          antlrcpp::downCast<SExpAgseContext *>(_localctx)->window = int_const();
          setState(402);
          match(RQLParser::T__3);
          break;
        }

        case 4: {
          auto newContext = _tracker.createInstance<SExpAndContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(404);

          if (!(precpred(_ctx, 8))) throw FailedPredicateException(this, "precpred(_ctx, 8)");
          setState(405);
          match(RQLParser::AND);
          setState(406);
          rational_se();
          break;
        }

        case 5: {
          auto newContext = _tracker.createInstance<SExpModContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(407);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(408);
          match(RQLParser::MOD);
          setState(409);
          rational_se();
          break;
        }

        case 6: {
          auto newContext = _tracker.createInstance<SExpTimeMoveContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(410);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(411);
          match(RQLParser::IS_GR);
          setState(412);
          int_const();
          break;
        }

        case 7: {
          auto newContext = _tracker.createInstance<SExpMinusContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(413);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(414);
          match(RQLParser::MINUS);
          setState(415);
          rational_se();
          break;
        }

        case 8: {
          auto newContext = _tracker.createInstance<SExpAgregate_proformaContext>(_tracker.createInstance<Stream_expressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleStream_expression);
          setState(416);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(417);
          match(RQLParser::DOT);
          setState(418);
          agregator();
          break;
        }

        default:
          break;
        } 
      }
      setState(423);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 43, _ctx);
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
  enterRule(_localctx, 60, RQLParser::RuleStream_factor);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(435);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 44, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(424);
      match(RQLParser::ID);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(425);
      match(RQLParser::ID);
      setState(426);
      match(RQLParser::T__0);
      setState(427);
      gen_index(0);
      setState(428);
      match(RQLParser::T__1);
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(430);
      match(RQLParser::T__2);
      setState(431);
      stream_expression(0);
      setState(432);
      match(RQLParser::T__3);
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(434);
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
  size_t startState = 62;
  enterRecursionRule(_localctx, 62, RQLParser::RuleGen_index, precedence);

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
    setState(445);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::T__2: {
        setState(438);
        match(RQLParser::T__2);
        setState(439);
        gen_index(0);
        setState(440);
        match(RQLParser::T__3);
        break;
      }

      case RQLParser::DOLLAR: {
        setState(442);
        match(RQLParser::DOLLAR);
        break;
      }

      case RQLParser::DECIMAL: {
        setState(443);
        match(RQLParser::DECIMAL);
        break;
      }

      case RQLParser::CONST_REF: {
        setState(444);
        match(RQLParser::CONST_REF);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    _ctx->stop = _input->LT(-1);
    setState(455);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 47, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(453);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 46, _ctx)) {
        case 1: {
          _localctx = _tracker.createInstance<Gen_indexContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleGen_index);
          setState(447);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(448);
          match(RQLParser::STAR);
          setState(449);
          gen_index(7);
          break;
        }

        case 2: {
          _localctx = _tracker.createInstance<Gen_indexContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleGen_index);
          setState(450);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(451);
          _la = _input->LA(1);
          if (!(_la == RQLParser::PLUS

          || _la == RQLParser::MINUS)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(452);
          gen_index(6);
          break;
        }

        default:
          break;
        } 
      }
      setState(457);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 47, _ctx);
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
  enterRule(_localctx, 64, RQLParser::RuleAgregator);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(462);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case RQLParser::MIN: {
        _localctx = _tracker.createInstance<RQLParser::StreamMinContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(458);
        match(RQLParser::MIN);
        break;
      }

      case RQLParser::MAX: {
        _localctx = _tracker.createInstance<RQLParser::StreamMaxContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(459);
        match(RQLParser::MAX);
        break;
      }

      case RQLParser::AVG: {
        _localctx = _tracker.createInstance<RQLParser::StreamAvgContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(460);
        match(RQLParser::AVG);
        break;
      }

      case RQLParser::SUMC: {
        _localctx = _tracker.createInstance<RQLParser::StreamSumContext>(_localctx);
        enterOuterAlt(_localctx, 4);
        setState(461);
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
  enterRule(_localctx, 66, RQLParser::RuleStream_fn_call);
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
    setState(464);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 263882790666240) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(465);
    match(RQLParser::T__2);
    setState(466);
    stream_expression(0);
    setState(467);
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
  enterRule(_localctx, 68, RQLParser::RuleWindow_agg);
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
    setState(469);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 263882790666240) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(470);
    match(RQLParser::T__2);
    setState(471);
    expression_factor(0);
    setState(472);
    match(RQLParser::COLON);
    setState(473);
    antlrcpp::downCast<Window_aggContext *>(_localctx)->width = int_const();
    setState(474);
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
  enterRule(_localctx, 70, RQLParser::RuleFunction_call);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(488);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 49, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(476);
      antlrcpp::downCast<Function_callContext *>(_localctx)->fn = match(RQLParser::ID);
      setState(477);
      match(RQLParser::T__2);
      setState(478);
      expression_factor(0);
      setState(479);
      match(RQLParser::T__3);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(481);
      antlrcpp::downCast<Function_callContext *>(_localctx)->fn = match(RQLParser::ID);
      setState(482);
      match(RQLParser::T__2);
      setState(483);
      expression_factor(0);
      setState(484);
      match(RQLParser::COLON);
      setState(485);
      antlrcpp::downCast<Function_callContext *>(_localctx)->width = int_const();
      setState(486);
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
    case 25: return expression_logicSempred(antlrcpp::downCast<Expression_logicContext *>(context), predicateIndex);
    case 26: return term_logicSempred(antlrcpp::downCast<Term_logicContext *>(context), predicateIndex);
    case 27: return expression_factorSempred(antlrcpp::downCast<Expression_factorContext *>(context), predicateIndex);
    case 28: return termSempred(antlrcpp::downCast<TermContext *>(context), predicateIndex);
    case 29: return stream_expressionSempred(antlrcpp::downCast<Stream_expressionContext *>(context), predicateIndex);
    case 31: return gen_indexSempred(antlrcpp::downCast<Gen_indexContext *>(context), predicateIndex);

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
