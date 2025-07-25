#include <rdb/convertTypes.h>
#include <spdlog/spdlog.h>

#include <boost/algorithm/string.hpp>
#include <boost/cerrno.hpp>
#include <boost/json.hpp>
#include <boost/lexical_cast.hpp>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include ".antlr/RQLBaseListener.h"
#include ".antlr/RQLLexer.h"
#include ".antlr/RQLParser.h"
#include "QStruct.h"
#include "antlr4-runtime/antlr4-runtime.h"

using namespace antlrcpp;
using namespace antlr4;

std::string status = "OK";

// https://stackoverflow.com/questions/44515370/how-to-override-error-reporting-in-c-target-of-antlr4

class LexerErrorListener : public BaseErrorListener {
 public:
  void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                   const std::string &msg, std::exception_ptr e) override {
    std::cerr << "Syntax error @Rql" << std::endl;
    std::cerr << "line:" << line << ":" << charPositionInLine << " at " << offendingSymbol << std::endl;
    std::cerr << "msg:" << msg << std::endl;
    status = "Fail";
    exit(EPERM);
  }
};

class ParserErrorListener : public BaseErrorListener {
 public:
  void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                   const std::string &msg, std::exception_ptr e) override {
    std::cerr << "Syntax error @Rql" << std::endl;
    std::cerr << "line:" << line << ":" << charPositionInLine << " at " << offendingSymbol << std::endl;
    std::cerr << "msg:" << msg << std::endl;
    status = "Fail";
    exit(EPERM);
  }
};

/* Iterator - each new field gets new fieldCount number */
int fieldCount = 0;

class ParserListener : public RQLBaseListener {
  qTree &coreInstance;

  /* Helper variable required for rational numbers processing */
  boost::rational<int> rationalResult;

  /* Helper variable required to build query or declaration */
  query qry;

  /* sequence of tokens - same variable for stream and field program*/
  std::list<token> program;

  /* Type of field */
  rdb::descFld fType = rdb::BYTE;

  /* Filed type */
  int fTypeSize = 1;

  /* Type of field - eq.1-atomic, >1 - array */
  int fTypeSizeArray = 1;

  void recpToken(command_id id) { program.push_back(token(id)); };

  template <typename T>
  void recpToken(command_id id, T arg1) {
    program.push_back(token(id, arg1));
  };

 public:
  ParserListener(qTree &coreInstance) : coreInstance(coreInstance){};

  void enterProg(RQLParser::ProgContext *ctx) {}

  void exitFieldID(RQLParser::FieldIDContext *ctx) { recpToken(PUSH_ID3, ctx->getText()); }
  void exitFieldIDUnderline(RQLParser::FieldIDUnderlineContext *ctx) { recpToken(PUSH_IDX, ctx->getText()); }
  void exitFieldIDColumnName(RQLParser::FieldIDColumnNameContext *ctx) { recpToken(PUSH_ID1, ctx->getText()); }
  void exitFieldIDTable(RQLParser::FieldIDTableContext *ctx) { recpToken(PUSH_ID2, ctx->getText()); }

  void exitExpPlus(RQLParser::ExpPlusContext *ctx) { recpToken(ADD); }
  void exitExpMinus(RQLParser::ExpMinusContext *ctx) { recpToken(SUBTRACT); }
  void exitExpMult(RQLParser::ExpMultContext *ctx) { recpToken(MULTIPLY); }
  void exitExpDiv(RQLParser::ExpDivContext *ctx) { recpToken(DIVIDE); }

  void exitExpFloat(RQLParser::ExpFloatContext *ctx) { recpToken(PUSH_VAL, std::stof(ctx->getText())); }
  void exitExpDec(RQLParser::ExpDecContext *ctx) { recpToken(PUSH_VAL, std::stoi(ctx->getText())); }
  void exitExpString(RQLParser::ExpStringContext *ctx) { program.push_back(token(PUSH_VAL, rdb::descFldVT(ctx->getText()))); }
  void exitExpRational(RQLParser::ExpRationalContext *ctx) { program.push_back(token(PUSH_VAL, rationalResult)); }

  void exitSExpHash(RQLParser::SExpHashContext *ctx) { recpToken(STREAM_HASH); }

  void exitSExpAnd(RQLParser::SExpAndContext *ctx) {
    recpToken(PUSH_VAL, rationalResult);
    recpToken(STREAM_DEHASH_DIV);
  }

  void exitSExpMod(RQLParser::SExpModContext *ctx) {
    recpToken(PUSH_VAL, rationalResult);
    recpToken(STREAM_DEHASH_MOD);
  }

  void exitStreamMin(RQLParser::StreamMinContext *ctx) { recpToken(STREAM_MIN); }
  void exitStreamMax(RQLParser::StreamMaxContext *ctx) { recpToken(STREAM_MAX); }
  void exitStreamAvg(RQLParser::StreamAvgContext *ctx) { recpToken(STREAM_AVG); }
  void exitStreamSum(RQLParser::StreamSumContext *ctx) { recpToken(STREAM_SUM); }
  void exitSExpPlus(RQLParser::SExpPlusContext *ctx) { recpToken(STREAM_ADD); }
  void exitSExpMinus(RQLParser::SExpMinusContext *ctx) { recpToken(STREAM_SUBTRACT, rationalResult); }

  void exitSExpAgse(RQLParser::SExpAgseContext *ctx) {
    int window{0}, step{0};
    if (ctx->children[5]->getText() == "-")
      window = -std::stoi(ctx->window->getText());
    else
      window = std::stoi(ctx->window->getText());
    step = std::stoi(ctx->step->getText());

    program.push_back(token(STREAM_AGSE, std::make_pair(step, window)));
  }

  void exitFunction_call(RQLParser::Function_callContext *ctx) { recpToken(CALL, ctx->children[0]->getText()); }

  // page 119 - The Definitive ANTL4 Reference Guide
  void exitDeclare(RQLParser::DeclareContext *ctx) {
    qry.filename = ctx->file_name->getText();
    // This removes ''
    qry.filename.erase(qry.filename.size() - 1);
    qry.filename.erase(0, 1);
    qry.id        = ctx->ID()->getText();
    qry.rInterval = rationalResult;
    coreInstance.push_back(qry);
    qry.reset();
    fieldCount = 0;
  }

  // https://www.programiz.com/cpp-programming/string-float-conversion
  // https://www.geeksforgeeks.org/converting-strings-numbers-cc/

  void exitRationalAsFloat(RQLParser::RationalAsFloatContext *ctx) {
    rationalResult = Rationalize(std::stod(ctx->FLOAT()->getText()));
  }

  void exitRationalAsDecimal(RQLParser::RationalAsDecimalContext *ctx) { rationalResult = std::stoi(ctx->DECIMAL()->getText()); }

  void exitFraction(RQLParser::FractionContext *ctx) {
    const int nom = std::stoi(ctx->children[0]->getText());
    const int den = std::stoi(ctx->children[2]->getText());
    assert(den != 0);
    rationalResult = boost::rational<int>(nom, den);
  }

  void exitSelect(RQLParser::SelectContext *ctx) {
    // this loop creates field names in streamName + "_" + counter++
    for (auto &i : qry.lSchema) {
      if ((i.field_.rname).substr(0, 1) == "_") (i.field_.rname) = ctx->ID()->getText() + i.field_.rname;
    }

    qry.id       = ctx->ID()->getText();
    qry.lProgram = program;
    coreInstance.push_back(qry);
    program.clear();
    qry.reset();
    fieldCount = 0;
  }

  void exitRetention(RQLParser::RetentionContext *ctx) {
    SPDLOG_INFO("Parser/Retention: {} {}",            //
                std::stoi(ctx->segments->getText()),  //
                std::stoi(ctx->capacity->getText()));
    qry.retention = std::pair<int, int>(      //
        std::stoi(ctx->segments->getText()),  //
        std::stoi(ctx->capacity->getText()));
  }

  void exitStorage(RQLParser::StorageContext *ctx) {
    qry.id       = ":STORAGE";
    qry.filename = ctx->folder_name->getText();
    // Remove ''
    qry.filename.erase(qry.filename.size() - 1);
    qry.filename.erase(0, 1);
    // Add / at the end of path
    if (qry.filename[qry.filename.size() - 1] != '/') qry.filename.push_back('/');
    // This should set paths correct on compiled system
    std::filesystem::path native_system_path{qry.filename};
    qry.filename = native_system_path.make_preferred().string();
    coreInstance.push_back(qry);
    program.clear();
    qry.reset();
    fieldCount = 0;
  }

  void exitSExpTimeMove(RQLParser::SExpTimeMoveContext *ctx) {
    recpToken(STREAM_TIMEMOVE, std::stoi(ctx->DECIMAL()->getText()));
  }

  void exitStream_factor(RQLParser::Stream_factorContext *ctx) {
    if (ctx->children.size() == 1) program.push_back(token(PUSH_STREAM, ctx->ID()->getText()));
  }

  void exitSelectListFullscan(RQLParser::SelectListFullscanContext *ctx) {
    recpToken(PUSH_TSCAN, ctx->getText());
    qry.lSchema.push_back(
        field(rdb::rField(/*Field_*/ "_" + boost::lexical_cast<std::string>(fieldCount++), 4, 1, rdb::INTEGER), program));
    program.clear();
  }

  void exitExpression(RQLParser::ExpressionContext *ctx) {
    qry.lSchema.push_back(
        field(rdb::rField(/*Field_*/ "_" + boost::lexical_cast<std::string>(fieldCount++), 4, 1, rdb::INTEGER), program));
    program.clear();
  }

  void exitTypeString(RQLParser::TypeStringContext *ctx) {
    fType     = rdb::STRING;
    fTypeSize = sizeof(uint8_t);
  }

  void exitTypeByte(RQLParser::TypeByteContext *ctx) {
    fType     = rdb::BYTE;
    fTypeSize = sizeof(uint8_t);
  }
  void exitTypeInt(RQLParser::TypeIntContext *ctx) {
    fType     = rdb::INTEGER;
    fTypeSize = sizeof(int);
  }
  void exitTypeUnsigned(RQLParser::TypeUnsignedContext *ctx) {
    fType     = rdb::UINT;
    fTypeSize = sizeof(unsigned);
  }
  void exitTypeFloat(RQLParser::TypeFloatContext *ctx) {
    fType     = rdb::FLOAT;
    fTypeSize = sizeof(float);
  }
  void exitTypeDouble(RQLParser::TypeDoubleContext *ctx) {
    fType     = rdb::DOUBLE;
    fTypeSize = sizeof(double);
  }

  void exitSingleDeclaration(RQLParser::SingleDeclarationContext *ctx) {
    auto fTypeSizeArray = 1;  // Default:1
    if (ctx->type_size) fTypeSizeArray = std::stoi(ctx->type_size->getText());
    std::list<token> emptyProgram;
    qry.lSchema.push_back(field(rdb::rField(ctx->ID()->getText(), fTypeSize, fTypeSizeArray, fType), emptyProgram));
    fType          = rdb::BYTE;
    fTypeSizeArray = 1;
  }
};

std::string parserRQLFile(qTree &coreInstance, std::string sInputFile) {
  std::ifstream ins;
  // Create the input stream.
  ins.open(sInputFile.c_str());
  ANTLRInputStream input(ins);
  // Create a lexer which scans the input stream
  // to create a token stream.
  RQLLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  LexerErrorListener lexerErrorListener;
  lexer.removeErrorListeners();
  lexer.addErrorListener(&lexerErrorListener);
  // Create a parser which parses the token stream
  // to create a parse tree.
  RQLParser parser(&tokens);
  ParserErrorListener parserErrorListener;
  ParserListener parserListener(coreInstance);
  parser.removeParseListeners();
  parser.removeErrorListeners();
  parser.addErrorListener(&parserErrorListener);
  parser.addParseListener(&parserListener);
  tree::ParseTree *tree = parser.prog();
  return status;
}

std::string parserRQLString(qTree &coreInstance, std::string inlet) {
  ANTLRInputStream input(inlet);
  // Create a lexer which scans the input stream
  // to create a token stream.
  RQLLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  LexerErrorListener lexerErrorListener;
  lexer.removeErrorListeners();
  lexer.addErrorListener(&lexerErrorListener);
  // Create a parser which parses the token stream
  // to create a parse tree.
  RQLParser parser(&tokens);
  ParserErrorListener parserErrorListener;
  ParserListener parserListener(coreInstance);
  parser.removeParseListeners();
  parser.removeErrorListeners();
  parser.addErrorListener(&parserErrorListener);
  parser.addParseListener(&parserListener);
  tree::ParseTree *tree = parser.prog();
  return status;
}
