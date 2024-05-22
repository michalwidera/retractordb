#include <iostream>

#include ".antlr/DESCBaseListener.h"
#include ".antlr/DESCLexer.h"
#include ".antlr/DESCParser.h"
#include "antlr4-runtime/antlr4-runtime.h"
#include "rdb/descriptor.h"

using namespace antlrcpp;
using namespace antlr4;

std::string statusDesc = "OK";

// https://stackoverflow.com/questions/44515370/how-to-override-error-reporting-in-c-target-of-antlr4

class LexerErrorListenerDesc : public BaseErrorListener {
 public:
  void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                   const std::string &msg, std::exception_ptr e) override {
    std::cerr << "Descriptor Syntax error - ";
    std::cerr << "line:" << line << ":" << charPositionInLine << " at " << offendingSymbol << ": " << msg;
    statusDesc = "Fail";
    exit(EPERM);
    return;
  }
};

class ParserErrorListenerDesc : public BaseErrorListener {
 public:
  void syntaxError(Recognizer *recognizer, Token *offendingSymbol, size_t line, size_t charPositionInLine,
                   const std::string &msg, std::exception_ptr e) override {
    std::cerr << "Descriptor Syntax error - ";
    std::cerr << "line:" << line << ":" << charPositionInLine << " at " << offendingSymbol << ": " << msg;
    statusDesc = "Fail";
    exit(EPERM);
    return;
  }
};

class ParserDESCListener : public DESCBaseListener {
  rdb::Descriptor &desc;

 public:
  ParserDESCListener(rdb::Descriptor &desc)
      : desc(desc){
            // std::cerr << "constructor" << std::endl;
        };

  void enterDesc(DESCParser::DescContext *ctx) {
    // std::cerr << "enterDesc" << std::endl;
  }

  void exitDesc(DESCParser::DescContext *ctx) {
    // std::cerr << "exitDesc:" << desc << std::endl;
  }

  void exitByteID(DESCParser::ByteIDContext *ctx) {
    int count = 1;
    if (ctx->arr) count = std::stoi(ctx->arr->getText());

    desc.append({rdb::rField(ctx->name->getText(), sizeof(uint8_t), count, rdb::BYTE)});
  }

  void exitIntegerID(DESCParser::IntegerIDContext *ctx) {
    int count = 1;
    if (ctx->arr) count = std::stoi(ctx->arr->getText());

    desc.append({rdb::rField(ctx->name->getText(), sizeof(int), count, rdb::INTEGER)});
  }

  void exitUnsignedID(DESCParser::UnsignedIDContext *ctx) {
    int count = 1;
    if (ctx->arr) count = std::stoi(ctx->arr->getText());

    desc.append({rdb::rField(ctx->name->getText(), sizeof(unsigned), count, rdb::UINT)});
  }

  void exitFloatID(DESCParser::FloatIDContext *ctx) {
    int count = 1;
    if (ctx->arr) count = std::stoi(ctx->arr->getText());

    desc.append({rdb::rField(ctx->name->getText(), sizeof(float), count, rdb::FLOAT)});
  }

  void exitDoubleID(DESCParser::DoubleIDContext *ctx) {
    int count = 1;
    if (ctx->arr) count = std::stoi(ctx->arr->getText());

    desc.append({rdb::rField(ctx->name->getText(), sizeof(double), count, rdb::DOUBLE)});
  }

  void exitRationalID(DESCParser::RationalIDContext *ctx) {
    int count = 1;
    if (ctx->arr) count = std::stoi(ctx->arr->getText());

    desc.append({rdb::rField(ctx->name->getText(), sizeof(boost::rational<int>), count, rdb::RATIONAL)});
  }

  void exitStringID(DESCParser::StringIDContext *ctx) {
    int count = std::stoi(ctx->strsize->getText());
    desc.append({rdb::rField(ctx->name->getText(), sizeof(char), count, rdb::STRING)});
  }

  void exitRefID(DESCParser::RefIDContext *ctx) { desc.append({rdb::rField(ctx->file->getText(), 0, 0, rdb::REF)}); }

  void exitTypeID(DESCParser::TypeIDContext *ctx) { desc.append({rdb::rField(ctx->type->getText(), 0, 0, rdb::TYPE)}); }

  void exitRetentionID(DESCParser::RetentionIDContext *ctx) {
    // len - capacity
    // arr - segments
    desc.append({rdb::rField("", std::stoi(ctx->capacity->getText()), std::stoi(ctx->segment->getText()), rdb::RETENTION)});
  }
};

std::string parserDESCString(rdb::Descriptor &desc, std::string inlet) {
  ANTLRInputStream input(inlet);
  // Create a lexer which scans the input stream
  // to create a token stream.
  DESCLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  LexerErrorListenerDesc lexerErrorListener;
  lexer.removeErrorListeners();
  lexer.addErrorListener(&lexerErrorListener);
  // Create a parser which parses the token stream
  // to create a parse tree.
  DESCParser parser(&tokens);
  ParserErrorListenerDesc parserErrorListener;
  ParserDESCListener parserDescListener(desc);
  parser.removeParseListeners();
  parser.removeErrorListeners();
  parser.addErrorListener(&parserErrorListener);
  parser.addParseListener(&parserDescListener);
  tree::ParseTree *tree = parser.desc();
  return statusDesc;
}
