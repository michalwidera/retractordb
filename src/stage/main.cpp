#include <iostream>
#include <cstdlib>
#include "antlr4-runtime/antlr4-runtime.h"

#include "Parser/RQLLexer.h"
#include "Parser/RQLParser.h"
#include "Parser/RQLBaseListener.h"
// #include "Parser/RQLBaseVisitor.h"

#include <boost/json.hpp>
#include <boost/cerrno.hpp>
#include <boost/lexical_cast.hpp>

#include "QStruct.h"

qTree coreInstance_parser ;
std::string sFieldName = "" ;
field::eType fieldType = field::BAD ;
stack <std::shared_ptr<query>> stk ;


#define RECPTOKEN(x)  { program.push_back( token( x ) ) ; }
#define RECPSTRTK(x)  { program.push_back( token( x , ctx->getText())) ; }

// antlr4 -o Parser -lib Parser -encoding UTF-8 -Dlanguage=Cpp -no-listener -visitor RQLParser.g4
// https://github.com/antlr/grammars-v4/tree/master/sql/tsql
// Regenerate grammar
// make grammar && make install && xstage ../src/stage/example_5.txt
//
// Generate grammar
// rm ../src/stage/Parser/*
// cd ../src/stage && antlr4call.sh RQL.g4 && cd ../../build
// :
// cd ../src/stage && sh -c ../../scripts/antlr4call.sh RQL.g4 && cd ../../build
//
// Check:
// xcompiler ../src/stage/example_5.txt -d && xdumper query.qry.lg1

namespace json = boost::json;

using namespace antlrcpp;
using namespace antlr4;

// https://stackoverflow.com/questions/44515370/how-to-override-error-reporting-in-c-target-of-antlr4

class LexerErrorListener : public BaseErrorListener
{
public:

    void syntaxError(Recognizer* recognizer, Token* offendingSymbol, size_t line, size_t charPositionInLine,
        const std::string &msg, std::exception_ptr e) override
    {
        std::cerr << "Syntax error - ";
        std::cerr << "line:" << line << ":" << charPositionInLine << " at " << offendingSymbol << ": " << msg ;
        exit(EPERM);
    }
};

class ParserErrorListener : public BaseErrorListener
{
public:

    void syntaxError(Recognizer* recognizer, Token* offendingSymbol, size_t line, size_t charPositionInLine,
        const std::string &msg, std::exception_ptr e) override
    {
        std::cerr << "Syntax error - " ;
        std::cerr << "line:" << line << ":" << charPositionInLine << " at " << offendingSymbol << ": " << msg ;
        exit(EPERM);
    }
};

class ParserListener : public RQLBaseListener
{

    /* Helper variable required for rational numbers processing */
    boost::rational<int> rationalResult;

    /* Helper variable required to build query or declaration */
    query qry;

    /* */
    list < token > program;

    int fieldCount;

    int typelen;

public:

    void enterProg(RQLParser::ProgContext* ctx)
    {
        fieldCount = 0;
    }

    void exitSelectList(RQLParser::SelectListContext* ctx)
    {
        //do_insert_into_schema
        for (auto i : ctx->expression())
        {
            std::cout << "## select item: " << i->getText() << std::endl ;
        }

        std::cout << "## exitSelectList" << std::endl;
        //
    }

    void exitFieldID(RQLParser::FieldIDContext* ctx) RECPSTRTK(PUSH_ID3)
    void exitFieldIDUnderline(RQLParser::FieldIDUnderlineContext* ctx) RECPSTRTK(PUSH_IDX)
    void exitFieldIDColumnname(RQLParser::FieldIDColumnnameContext* ctx) RECPSTRTK(PUSH_ID1)
    void exitFieldIDTable(RQLParser::FieldIDTableContext* ctx) RECPSTRTK(PUSH_ID2)

    void exitExpPlus(RQLParser::ExpPlusContext* ctx) RECPTOKEN(ADD)
    void exitExpMinus(RQLParser::ExpMinusContext* ctx) RECPTOKEN(SUBTRACT)
    void exitExpMult(RQLParser::ExpMultContext* ctx) RECPTOKEN(MULTIPLY)
    void exitExpDiv(RQLParser::ExpDivContext* ctx) RECPTOKEN(DIVIDE)

    void exitExpDec(RQLParser::ExpDecContext* ctx)
    {
        program.push_back(token(PUSH_VAL, ctx->DECIMAL()->getText()));
    }

    // page 119 - The Definitive ANTL4 Reference Guide
    void exitDeclare(RQLParser::DeclareContext* ctx)
    {
        //do_insert_into_schema
        std::cout << "&&" << __func__ << "{" << std::endl;

        std::cout << "size:" << ctx->children.size() << std::endl ;

        for (auto a : ctx->children)
        {
            std::cout << "  |" << a->getText() << std::endl ;
        }

        std::cout << ctx->stream_name->getText() << std::endl << "  ";
        std::cout << ctx->declare_list()->getText() << std::endl << "  ";
        std::cout << ctx->ID()->getText() << std::endl << "  ";  // source
        std::cout << ctx->STRING()->getText() << std::endl << "  ";
        std::cout << ctx->children[0]->getText() << std::endl << "  "; // DECLARE   values.get(ctx.getChild(0))
        std::cout << "}" << __func__ << std::endl;

        qry.filename = ctx->file_name->getText();
        qry.id = ctx->ID()->getText();
        qry.rInterval = rationalResult;
        coreInstance_parser.push_back(qry);
        qry.reset();
    }

    // https://www.programiz.com/cpp-programming/string-float-conversion
    // https://www.geeksforgeeks.org/converting-strings-numbers-cc/

    void exitRationalAsFloat(RQLParser::RationalAsFloatContext* ctx)
    {
        rationalResult = Rationalize(std::stod(ctx->FLOAT()->getText()));
    }

    void exitRationalAsDecimal(RQLParser::RationalAsDecimalContext* ctx)
    {
        rationalResult = std::stoi(ctx->DECIMAL()->getText());
    }

    void exitFraction(RQLParser::FractionContext* ctx)
    {
        const int nom = std::stoi(ctx->children[0]->getText());
        const int den = std::stoi(ctx->children[2]->getText());
        rationalResult = boost::rational<int>(nom, den);
    }

    void exitSelect(RQLParser::SelectContext* ctx)
    {
        std::cout << "&&" << __func__ << "{" << std::endl;

        for (auto a : ctx->children)
        {
            std::cout << "  |" << a->getText() << std::endl;
        }

        std::cout << "}" << __func__ << std::endl;
        qry.id = ctx->ID()->getText();
        qry.lProgram = program;
        coreInstance_parser.push_back(qry);
        qry.reset();
    }

    void exitSExpTimeMove(RQLParser::SExpTimeMoveContext* ctx)
    {
        program.push_back(token(STREAM_TIMEMOVE, std::stoi(ctx->DECIMAL()->getText())));
    }

    void exitStream_factor(RQLParser::Stream_factorContext* ctx)
    {
        if (ctx->children.size() == 1)
        {
            program.push_back(token(PUSH_STREAM, ctx->ID()->getText()));
        }
    }

    void exitExpression(RQLParser::ExpressionContext* ctx)
    {
        std::cout << "&&" << __func__ << "{" << std::endl;

        for (auto a : ctx->children)
        {
            std::cout << "  |" << a->getText() << std::endl ;
        }

        std::cout << "}" << __func__ << std::endl;
        qry.lSchema.push_back(field("Field_" + boost::lexical_cast<std::string> (fieldCount ++), program, field::INTEGER, "todo 2"));
        program.clear();
    }

    void exitSingleDeclaration(RQLParser::SingleDeclarationContext* ctx)
    {
        std::cout << "&&" << __func__ << "{" << std::endl;
        std::cout << " ID: " << ctx->ID()->getText() ;
        std::cout << ",type: " << ctx->field_type()->getText() << "}" << std::endl;

        list < token > emptyProgram;
        qry.lSchema.push_back(field(ctx->ID()->getText(), emptyProgram, field::INTEGER, "todo 3"));
    }

    // Working here - trying to unificate rdb types and RQL types

    void exitField_type(RQLParser::Field_typeContext *ctx)
    {
        std::cout << "&&" << __func__ << "{" << std::endl;

        if (ctx->children.size() == 1)
        {
            typelen = 1;
        }
        else
        {
            assert(ctx->children.size() == 4);
            typelen = std::stoi(ctx->children[2]->getText());
        }

        std::cout << "typelen:" << typelen << std::endl;

        for (auto a : ctx->children)
        {
            std::cout << "  |" << a->getText() << std::endl;
        }

        std::cout << "}" << __func__ << std::endl;
    }
};

int main(int argc, const char* args[])
{
    /*
    json::object obj;                                                     // construct an empty object
    obj[ "pi" ] = 3.141;                                            // insert a double
    obj[ "happy" ] = true;                                          // insert a bool
    obj[ "name" ] = "Boost";                                        // insert a string
    obj[ "nothing" ] = nullptr;                                     // insert a null
    obj[ "answer" ].emplace_object()["everything"] = {{"test2",42}} ;            // insert an object with 2 element
    obj[ "list" ] = { 1, 0, 2 };                                    // insert an array with 3 elements
    obj[ "object" ] = { {"currency", "USD"}, {"value", 42.99} };    // insert an object with 2 elements

    json::object obj2;
    obj2["test2"] = "1";
    obj2["test3"] = "Test2";

    obj["insert"] = obj2;

    std::cout << obj << std::endl ;

    json::value jv = {
            { "pi", 3.141 },
            { "happy", true },
            { "name", "Boost" },
            { "nothing", nullptr },
            { "answer", {
                            { "everything", {{"test2",42}} } } },
            {"list", {1, 0, 2}},
            {"object", {
                            { "currency", "USD" },
                         { "value", 42.99 }
                    } }
    };

    std::cout << jv << std::endl ;
    */
    std::ifstream ins;
    // Create the input stream.
    ins.open(args[1]);
    ANTLRInputStream input(ins);
    // Create a lexer which scans the input stream
    // to create a token stream.
    RQLLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    LexerErrorListener lexerErrorListener;
    lexer.removeErrorListeners();
    lexer.addErrorListener(&lexerErrorListener);
    /*
        // Print the token stream.
        cout << "Tokens:" << endl;
        tokens.fill();

        for (Token *token : tokens.getTokens())
        {
            std::cout << token->toString() << std::endl;
        }
    */
    // Create a parser which parses the token stream
    // to create a parse tree.
    RQLParser parser(&tokens);
    ParserErrorListener parserErrorListener;
    ParserListener parserListener;
    parser.removeParseListeners();
    parser.removeErrorListeners();
    parser.addErrorListener(&parserErrorListener);
    parser.addParseListener(&parserListener);
    tree::ParseTree* tree = parser.prog();
    // Print the parse tree in Lisp format.
    std::cout << std::endl << "Parse tree (Lisp format):" << std::endl;
    std::cout << tree->toStringTree(&parser) << std::endl;
    std::cout << std::endl ;
    // This code is for test purposes only
    // Remove before merge
    {
        const qTree coreInstance2(coreInstance_parser) ;
        std::ofstream ofs("query-stg.qry");
        boost::archive::text_oarchive oa(ofs);
        oa << coreInstance2 ;
    } // Destructor here will create query-stg.qry file so std::system can see it in next line.
    std::system("xdumper -i query-stg.qry");
    std::remove("query-stg.qry");
    return 0;
}