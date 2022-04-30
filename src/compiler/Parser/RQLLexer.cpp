
// Generated from RQL.g4 by ANTLR 4.10.1


#include "RQLLexer.h"


using namespace antlr4;



using namespace antlr4;

namespace
{

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

        RQLLexerStaticData(const RQLLexerStaticData &) = delete;
        RQLLexerStaticData(RQLLexerStaticData &&) = delete;
        RQLLexerStaticData &operator=(const RQLLexerStaticData &) = delete;
        RQLLexerStaticData &operator=(RQLLexerStaticData &&) = delete;

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
    RQLLexerStaticData* rqllexerLexerStaticData = nullptr;

    void rqllexerLexerInitialize()
    {
        assert(rqllexerLexerStaticData == nullptr);
        auto staticData = std::make_unique<RQLLexerStaticData>(
        std::vector<std::string> {
            "T__0", "T__1", "T__2", "T__3", "T__4", "T__5", "T__6", "T__7", "T__8",
            "T__9", "T__10", "T__11", "T__12", "T__13", "T__14", "T__15", "T__16",
            "T__17", "T__18", "T__19", "STRING_T", "BYTEARRAY_T", "INTARRAY_T",
            "BYTE_T", "UNSIGNED_T", "INTEGER_T", "FLOAT_T", "SELECT", "STREAM",
            "FROM", "DECLARE", "FILE", "STORAGE", "MIN", "MAX", "AVG", "SUMC",
            "ID", "STRING", "FLOAT", "DECIMAL", "REAL", "EQUAL", "GREATER", "LESS",
            "EXCLAMATION", "DOUBLE_BAR", "DOT", "UNDERLINE", "AT", "SHARP", "AND",
            "MOD", "DOLLAR", "COMMA", "SEMI", "COLON", "DOUBLE_COLON", "STAR",
            "DIVIDE", "PLUS", "MINUS", "BIT_NOT", "BIT_OR", "BIT_XOR", "SPACE",
            "COMMENT", "LINE_COMMENT", "LETTER", "DEC_DOT_DEC", "HEX_DIGIT", "DEC_DIGIT"
        },
        std::vector<std::string> {
            "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
        },
        std::vector<std::string> {
            "DEFAULT_MODE"
        },
        std::vector<std::string> {
            "", "'['", "']'", "'('", "')'", "'Sqrt'", "'Ceil'", "'Abs'", "'Floor'",
            "'Sign'", "'Chr'", "'Length'", "'ToNumber'", "'ToTimeStamp'", "'FloatCast'",
            "'InstCast'", "'Count'", "'Crc'", "'Sum'", "'IsZero'", "'IsNonZero'",
            "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
            "", "", "", "", "", "'='", "'>'", "'<'", "'!'", "'||'", "'.'", "'_'",
            "'@'", "'#'", "'&'", "'%'", "'$'", "','", "';'", "':'", "'::'", "'*'",
            "'/'", "'+'", "'-'", "'~'", "'|'", "'^'"
        },
        std::vector<std::string> {
            "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
            "", "", "", "", "STRING_T", "BYTEARRAY_T", "INTARRAY_T", "BYTE_T",
            "UNSIGNED_T", "INTEGER_T", "FLOAT_T", "SELECT", "STREAM", "FROM",
            "DECLARE", "FILE", "STORAGE", "MIN", "MAX", "AVG", "SUMC", "ID", "STRING",
            "FLOAT", "DECIMAL", "REAL", "EQUAL", "GREATER", "LESS", "EXCLAMATION",
            "DOUBLE_BAR", "DOT", "UNDERLINE", "AT", "SHARP", "AND", "MOD", "DOLLAR",
            "COMMA", "SEMI", "COLON", "DOUBLE_COLON", "STAR", "DIVIDE", "PLUS",
            "MINUS", "BIT_NOT", "BIT_OR", "BIT_XOR", "SPACE", "COMMENT", "LINE_COMMENT"
        }
            );
        static const int32_t serializedATNSegment[] = {
            4, 0, 68, 628, 6, -1, 2, 0, 7, 0, 2, 1, 7, 1, 2, 2, 7, 2, 2, 3, 7, 3, 2, 4, 7, 4, 2, 5, 7, 5, 2, 6, 7,
            6, 2, 7, 7, 7, 2, 8, 7, 8, 2, 9, 7, 9, 2, 10, 7, 10, 2, 11, 7, 11, 2, 12, 7, 12, 2, 13, 7, 13, 2, 14,
            7, 14, 2, 15, 7, 15, 2, 16, 7, 16, 2, 17, 7, 17, 2, 18, 7, 18, 2, 19, 7, 19, 2, 20, 7, 20, 2, 21,
            7, 21, 2, 22, 7, 22, 2, 23, 7, 23, 2, 24, 7, 24, 2, 25, 7, 25, 2, 26, 7, 26, 2, 27, 7, 27, 2, 28,
            7, 28, 2, 29, 7, 29, 2, 30, 7, 30, 2, 31, 7, 31, 2, 32, 7, 32, 2, 33, 7, 33, 2, 34, 7, 34, 2, 35,
            7, 35, 2, 36, 7, 36, 2, 37, 7, 37, 2, 38, 7, 38, 2, 39, 7, 39, 2, 40, 7, 40, 2, 41, 7, 41, 2, 42,
            7, 42, 2, 43, 7, 43, 2, 44, 7, 44, 2, 45, 7, 45, 2, 46, 7, 46, 2, 47, 7, 47, 2, 48, 7, 48, 2, 49,
            7, 49, 2, 50, 7, 50, 2, 51, 7, 51, 2, 52, 7, 52, 2, 53, 7, 53, 2, 54, 7, 54, 2, 55, 7, 55, 2, 56,
            7, 56, 2, 57, 7, 57, 2, 58, 7, 58, 2, 59, 7, 59, 2, 60, 7, 60, 2, 61, 7, 61, 2, 62, 7, 62, 2, 63,
            7, 63, 2, 64, 7, 64, 2, 65, 7, 65, 2, 66, 7, 66, 2, 67, 7, 67, 2, 68, 7, 68, 2, 69, 7, 69, 2, 70,
            7, 70, 2, 71, 7, 71, 1, 0, 1, 0, 1, 1, 1, 1, 1, 2, 1, 2, 1, 3, 1, 3, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 1,
            5, 1, 5, 1, 5, 1, 5, 1, 5, 1, 6, 1, 6, 1, 6, 1, 6, 1, 7, 1, 7, 1, 7, 1, 7, 1, 7, 1, 7, 1, 8, 1, 8, 1, 8,
            1, 8, 1, 8, 1, 9, 1, 9, 1, 9, 1, 9, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 11, 1, 11,
            1, 11, 1, 11, 1, 11, 1, 11, 1, 11, 1, 11, 1, 11, 1, 12, 1, 12, 1, 12, 1, 12, 1, 12, 1, 12, 1, 12,
            1, 12, 1, 12, 1, 12, 1, 12, 1, 12, 1, 13, 1, 13, 1, 13, 1, 13, 1, 13, 1, 13, 1, 13, 1, 13, 1, 13,
            1, 13, 1, 14, 1, 14, 1, 14, 1, 14, 1, 14, 1, 14, 1, 14, 1, 14, 1, 14, 1, 15, 1, 15, 1, 15, 1, 15,
            1, 15, 1, 15, 1, 16, 1, 16, 1, 16, 1, 16, 1, 17, 1, 17, 1, 17, 1, 17, 1, 18, 1, 18, 1, 18, 1, 18,
            1, 18, 1, 18, 1, 18, 1, 19, 1, 19, 1, 19, 1, 19, 1, 19, 1, 19, 1, 19, 1, 19, 1, 19, 1, 19, 1, 20,
            1, 20, 1, 20, 1, 20, 1, 20, 1, 20, 1, 20, 1, 20, 1, 20, 1, 20, 1, 20, 1, 20, 3, 20, 273, 8, 20,
            1, 21, 1, 21, 1, 21, 1, 21, 1, 21, 1, 21, 1, 21, 1, 21, 1, 21, 1, 21, 1, 21, 1, 21, 1, 21, 1, 21,
            1, 21, 1, 21, 1, 21, 1, 21, 3, 21, 293, 8, 21, 1, 22, 1, 22, 1, 22, 1, 22, 1, 22, 1, 22, 1, 22,
            1, 22, 1, 22, 1, 22, 1, 22, 1, 22, 1, 22, 1, 22, 1, 22, 1, 22, 3, 22, 311, 8, 22, 1, 23, 1, 23,
            1, 23, 1, 23, 1, 23, 1, 23, 1, 23, 1, 23, 3, 23, 321, 8, 23, 1, 24, 1, 24, 1, 24, 1, 24, 1, 24,
            1, 24, 1, 24, 1, 24, 1, 24, 1, 24, 1, 24, 1, 24, 3, 24, 335, 8, 24, 1, 25, 1, 25, 1, 25, 1, 25,
            1, 25, 1, 25, 1, 25, 1, 25, 1, 25, 1, 25, 1, 25, 1, 25, 1, 25, 3, 25, 350, 8, 25, 1, 26, 1, 26,
            1, 26, 1, 26, 1, 26, 1, 26, 1, 26, 1, 26, 1, 26, 1, 26, 3, 26, 362, 8, 26, 1, 27, 1, 27, 1, 27,
            1, 27, 1, 27, 1, 27, 1, 27, 1, 27, 1, 27, 1, 27, 1, 27, 1, 27, 3, 27, 376, 8, 27, 1, 28, 1, 28,
            1, 28, 1, 28, 1, 28, 1, 28, 1, 28, 1, 28, 1, 28, 1, 28, 1, 28, 1, 28, 3, 28, 390, 8, 28, 1, 29,
            1, 29, 1, 29, 1, 29, 1, 29, 1, 29, 1, 29, 1, 29, 3, 29, 400, 8, 29, 1, 30, 1, 30, 1, 30, 1, 30,
            1, 30, 1, 30, 1, 30, 1, 30, 1, 30, 1, 30, 1, 30, 1, 30, 1, 30, 1, 30, 3, 30, 416, 8, 30, 1, 31,
            1, 31, 1, 31, 1, 31, 1, 31, 1, 31, 1, 31, 1, 31, 3, 31, 426, 8, 31, 1, 32, 1, 32, 1, 32, 1, 32,
            1, 32, 1, 32, 1, 32, 1, 32, 1, 32, 1, 32, 1, 32, 1, 32, 1, 32, 1, 32, 3, 32, 442, 8, 32, 1, 33,
            1, 33, 1, 33, 1, 33, 1, 33, 1, 33, 3, 33, 450, 8, 33, 1, 34, 1, 34, 1, 34, 1, 34, 1, 34, 1, 34,
            3, 34, 458, 8, 34, 1, 35, 1, 35, 1, 35, 1, 35, 1, 35, 1, 35, 3, 35, 466, 8, 35, 1, 36, 1, 36, 1,
            36, 1, 36, 1, 36, 1, 36, 1, 36, 1, 36, 3, 36, 476, 8, 36, 1, 37, 1, 37, 5, 37, 480, 8, 37, 10,
            37, 12, 37, 483, 9, 37, 1, 38, 1, 38, 1, 38, 1, 38, 5, 38, 489, 8, 38, 10, 38, 12, 38, 492, 9,
            38, 1, 38, 1, 38, 1, 39, 1, 39, 1, 40, 4, 40, 499, 8, 40, 11, 40, 12, 40, 500, 1, 41, 1, 41, 3,
            41, 505, 8, 41, 1, 41, 1, 41, 3, 41, 509, 8, 41, 1, 41, 4, 41, 512, 8, 41, 11, 41, 12, 41, 513,
            1, 42, 1, 42, 1, 43, 1, 43, 1, 44, 1, 44, 1, 45, 1, 45, 1, 46, 1, 46, 1, 46, 1, 47, 1, 47, 1, 48,
            1, 48, 1, 49, 1, 49, 1, 50, 1, 50, 1, 51, 1, 51, 1, 52, 1, 52, 1, 53, 1, 53, 1, 54, 1, 54, 1, 55,
            1, 55, 1, 56, 1, 56, 1, 57, 1, 57, 1, 57, 1, 58, 1, 58, 1, 59, 1, 59, 1, 60, 1, 60, 1, 61, 1, 61,
            1, 62, 1, 62, 1, 63, 1, 63, 1, 64, 1, 64, 1, 65, 4, 65, 565, 8, 65, 11, 65, 12, 65, 566, 1, 65,
            1, 65, 1, 66, 1, 66, 1, 66, 1, 66, 1, 66, 5, 66, 576, 8, 66, 10, 66, 12, 66, 579, 9, 66, 1, 66,
            1, 66, 1, 66, 1, 66, 1, 66, 1, 67, 1, 67, 1, 67, 1, 67, 5, 67, 590, 8, 67, 10, 67, 12, 67, 593,
            9, 67, 1, 67, 1, 67, 1, 68, 1, 68, 1, 69, 4, 69, 600, 8, 69, 11, 69, 12, 69, 601, 1, 69, 1, 69,
            4, 69, 606, 8, 69, 11, 69, 12, 69, 607, 1, 69, 4, 69, 611, 8, 69, 11, 69, 12, 69, 612, 1, 69,
            1, 69, 1, 69, 1, 69, 4, 69, 619, 8, 69, 11, 69, 12, 69, 620, 3, 69, 623, 8, 69, 1, 70, 1, 70,
            1, 71, 1, 71, 1, 577, 0, 72, 1, 1, 3, 2, 5, 3, 7, 4, 9, 5, 11, 6, 13, 7, 15, 8, 17, 9, 19, 10, 21,
            11, 23, 12, 25, 13, 27, 14, 29, 15, 31, 16, 33, 17, 35, 18, 37, 19, 39, 20, 41, 21, 43, 22,
            45, 23, 47, 24, 49, 25, 51, 26, 53, 27, 55, 28, 57, 29, 59, 30, 61, 31, 63, 32, 65, 33, 67,
            34, 69, 35, 71, 36, 73, 37, 75, 38, 77, 39, 79, 40, 81, 41, 83, 42, 85, 43, 87, 44, 89, 45,
            91, 46, 93, 47, 95, 48, 97, 49, 99, 50, 101, 51, 103, 52, 105, 53, 107, 54, 109, 55, 111,
            56, 113, 57, 115, 58, 117, 59, 119, 60, 121, 61, 123, 62, 125, 63, 127, 64, 129, 65, 131,
            66, 133, 67, 135, 68, 137, 0, 139, 0, 141, 0, 143, 0, 1, 0, 9, 2, 0, 65, 90, 97, 122, 5, 0, 36,
            36, 48, 57, 65, 90, 95, 95, 97, 122, 1, 0, 39, 39, 2, 0, 43, 43, 45, 45, 3, 0, 9, 10, 13, 13,
            32, 32, 2, 0, 10, 10, 13, 13, 2, 0, 65, 90, 95, 95, 2, 0, 48, 57, 65, 70, 1, 0, 48, 57, 658, 0,
            1, 1, 0, 0, 0, 0, 3, 1, 0, 0, 0, 0, 5, 1, 0, 0, 0, 0, 7, 1, 0, 0, 0, 0, 9, 1, 0, 0, 0, 0, 11, 1, 0, 0,
            0, 0, 13, 1, 0, 0, 0, 0, 15, 1, 0, 0, 0, 0, 17, 1, 0, 0, 0, 0, 19, 1, 0, 0, 0, 0, 21, 1, 0, 0, 0, 0,
            23, 1, 0, 0, 0, 0, 25, 1, 0, 0, 0, 0, 27, 1, 0, 0, 0, 0, 29, 1, 0, 0, 0, 0, 31, 1, 0, 0, 0, 0, 33, 1,
            0, 0, 0, 0, 35, 1, 0, 0, 0, 0, 37, 1, 0, 0, 0, 0, 39, 1, 0, 0, 0, 0, 41, 1, 0, 0, 0, 0, 43, 1, 0, 0,
            0, 0, 45, 1, 0, 0, 0, 0, 47, 1, 0, 0, 0, 0, 49, 1, 0, 0, 0, 0, 51, 1, 0, 0, 0, 0, 53, 1, 0, 0, 0, 0,
            55, 1, 0, 0, 0, 0, 57, 1, 0, 0, 0, 0, 59, 1, 0, 0, 0, 0, 61, 1, 0, 0, 0, 0, 63, 1, 0, 0, 0, 0, 65, 1,
            0, 0, 0, 0, 67, 1, 0, 0, 0, 0, 69, 1, 0, 0, 0, 0, 71, 1, 0, 0, 0, 0, 73, 1, 0, 0, 0, 0, 75, 1, 0, 0,
            0, 0, 77, 1, 0, 0, 0, 0, 79, 1, 0, 0, 0, 0, 81, 1, 0, 0, 0, 0, 83, 1, 0, 0, 0, 0, 85, 1, 0, 0, 0, 0,
            87, 1, 0, 0, 0, 0, 89, 1, 0, 0, 0, 0, 91, 1, 0, 0, 0, 0, 93, 1, 0, 0, 0, 0, 95, 1, 0, 0, 0, 0, 97, 1,
            0, 0, 0, 0, 99, 1, 0, 0, 0, 0, 101, 1, 0, 0, 0, 0, 103, 1, 0, 0, 0, 0, 105, 1, 0, 0, 0, 0, 107, 1,
            0, 0, 0, 0, 109, 1, 0, 0, 0, 0, 111, 1, 0, 0, 0, 0, 113, 1, 0, 0, 0, 0, 115, 1, 0, 0, 0, 0, 117, 1,
            0, 0, 0, 0, 119, 1, 0, 0, 0, 0, 121, 1, 0, 0, 0, 0, 123, 1, 0, 0, 0, 0, 125, 1, 0, 0, 0, 0, 127, 1,
            0, 0, 0, 0, 129, 1, 0, 0, 0, 0, 131, 1, 0, 0, 0, 0, 133, 1, 0, 0, 0, 0, 135, 1, 0, 0, 0, 1, 145, 1,
            0, 0, 0, 3, 147, 1, 0, 0, 0, 5, 149, 1, 0, 0, 0, 7, 151, 1, 0, 0, 0, 9, 153, 1, 0, 0, 0, 11, 158,
            1, 0, 0, 0, 13, 163, 1, 0, 0, 0, 15, 167, 1, 0, 0, 0, 17, 173, 1, 0, 0, 0, 19, 178, 1, 0, 0, 0, 21,
            182, 1, 0, 0, 0, 23, 189, 1, 0, 0, 0, 25, 198, 1, 0, 0, 0, 27, 210, 1, 0, 0, 0, 29, 220, 1, 0, 0,
            0, 31, 229, 1, 0, 0, 0, 33, 235, 1, 0, 0, 0, 35, 239, 1, 0, 0, 0, 37, 243, 1, 0, 0, 0, 39, 250,
            1, 0, 0, 0, 41, 272, 1, 0, 0, 0, 43, 292, 1, 0, 0, 0, 45, 310, 1, 0, 0, 0, 47, 320, 1, 0, 0, 0, 49,
            334, 1, 0, 0, 0, 51, 349, 1, 0, 0, 0, 53, 361, 1, 0, 0, 0, 55, 375, 1, 0, 0, 0, 57, 389, 1, 0, 0,
            0, 59, 399, 1, 0, 0, 0, 61, 415, 1, 0, 0, 0, 63, 425, 1, 0, 0, 0, 65, 441, 1, 0, 0, 0, 67, 449,
            1, 0, 0, 0, 69, 457, 1, 0, 0, 0, 71, 465, 1, 0, 0, 0, 73, 475, 1, 0, 0, 0, 75, 477, 1, 0, 0, 0, 77,
            484, 1, 0, 0, 0, 79, 495, 1, 0, 0, 0, 81, 498, 1, 0, 0, 0, 83, 504, 1, 0, 0, 0, 85, 515, 1, 0, 0,
            0, 87, 517, 1, 0, 0, 0, 89, 519, 1, 0, 0, 0, 91, 521, 1, 0, 0, 0, 93, 523, 1, 0, 0, 0, 95, 526,
            1, 0, 0, 0, 97, 528, 1, 0, 0, 0, 99, 530, 1, 0, 0, 0, 101, 532, 1, 0, 0, 0, 103, 534, 1, 0, 0, 0,
            105, 536, 1, 0, 0, 0, 107, 538, 1, 0, 0, 0, 109, 540, 1, 0, 0, 0, 111, 542, 1, 0, 0, 0, 113, 544,
            1, 0, 0, 0, 115, 546, 1, 0, 0, 0, 117, 549, 1, 0, 0, 0, 119, 551, 1, 0, 0, 0, 121, 553, 1, 0, 0,
            0, 123, 555, 1, 0, 0, 0, 125, 557, 1, 0, 0, 0, 127, 559, 1, 0, 0, 0, 129, 561, 1, 0, 0, 0, 131,
            564, 1, 0, 0, 0, 133, 570, 1, 0, 0, 0, 135, 585, 1, 0, 0, 0, 137, 596, 1, 0, 0, 0, 139, 622, 1,
            0, 0, 0, 141, 624, 1, 0, 0, 0, 143, 626, 1, 0, 0, 0, 145, 146, 5, 91, 0, 0, 146, 2, 1, 0, 0, 0,
            147, 148, 5, 93, 0, 0, 148, 4, 1, 0, 0, 0, 149, 150, 5, 40, 0, 0, 150, 6, 1, 0, 0, 0, 151, 152,
            5, 41, 0, 0, 152, 8, 1, 0, 0, 0, 153, 154, 5, 83, 0, 0, 154, 155, 5, 113, 0, 0, 155, 156, 5, 114,
            0, 0, 156, 157, 5, 116, 0, 0, 157, 10, 1, 0, 0, 0, 158, 159, 5, 67, 0, 0, 159, 160, 5, 101, 0,
            0, 160, 161, 5, 105, 0, 0, 161, 162, 5, 108, 0, 0, 162, 12, 1, 0, 0, 0, 163, 164, 5, 65, 0, 0,
            164, 165, 5, 98, 0, 0, 165, 166, 5, 115, 0, 0, 166, 14, 1, 0, 0, 0, 167, 168, 5, 70, 0, 0, 168,
            169, 5, 108, 0, 0, 169, 170, 5, 111, 0, 0, 170, 171, 5, 111, 0, 0, 171, 172, 5, 114, 0, 0, 172,
            16, 1, 0, 0, 0, 173, 174, 5, 83, 0, 0, 174, 175, 5, 105, 0, 0, 175, 176, 5, 103, 0, 0, 176, 177,
            5, 110, 0, 0, 177, 18, 1, 0, 0, 0, 178, 179, 5, 67, 0, 0, 179, 180, 5, 104, 0, 0, 180, 181, 5,
            114, 0, 0, 181, 20, 1, 0, 0, 0, 182, 183, 5, 76, 0, 0, 183, 184, 5, 101, 0, 0, 184, 185, 5, 110,
            0, 0, 185, 186, 5, 103, 0, 0, 186, 187, 5, 116, 0, 0, 187, 188, 5, 104, 0, 0, 188, 22, 1, 0,
            0, 0, 189, 190, 5, 84, 0, 0, 190, 191, 5, 111, 0, 0, 191, 192, 5, 78, 0, 0, 192, 193, 5, 117,
            0, 0, 193, 194, 5, 109, 0, 0, 194, 195, 5, 98, 0, 0, 195, 196, 5, 101, 0, 0, 196, 197, 5, 114,
            0, 0, 197, 24, 1, 0, 0, 0, 198, 199, 5, 84, 0, 0, 199, 200, 5, 111, 0, 0, 200, 201, 5, 84, 0,
            0, 201, 202, 5, 105, 0, 0, 202, 203, 5, 109, 0, 0, 203, 204, 5, 101, 0, 0, 204, 205, 5, 83,
            0, 0, 205, 206, 5, 116, 0, 0, 206, 207, 5, 97, 0, 0, 207, 208, 5, 109, 0, 0, 208, 209, 5, 112,
            0, 0, 209, 26, 1, 0, 0, 0, 210, 211, 5, 70, 0, 0, 211, 212, 5, 108, 0, 0, 212, 213, 5, 111, 0,
            0, 213, 214, 5, 97, 0, 0, 214, 215, 5, 116, 0, 0, 215, 216, 5, 67, 0, 0, 216, 217, 5, 97, 0,
            0, 217, 218, 5, 115, 0, 0, 218, 219, 5, 116, 0, 0, 219, 28, 1, 0, 0, 0, 220, 221, 5, 73, 0, 0,
            221, 222, 5, 110, 0, 0, 222, 223, 5, 115, 0, 0, 223, 224, 5, 116, 0, 0, 224, 225, 5, 67, 0,
            0, 225, 226, 5, 97, 0, 0, 226, 227, 5, 115, 0, 0, 227, 228, 5, 116, 0, 0, 228, 30, 1, 0, 0, 0,
            229, 230, 5, 67, 0, 0, 230, 231, 5, 111, 0, 0, 231, 232, 5, 117, 0, 0, 232, 233, 5, 110, 0,
            0, 233, 234, 5, 116, 0, 0, 234, 32, 1, 0, 0, 0, 235, 236, 5, 67, 0, 0, 236, 237, 5, 114, 0, 0,
            237, 238, 5, 99, 0, 0, 238, 34, 1, 0, 0, 0, 239, 240, 5, 83, 0, 0, 240, 241, 5, 117, 0, 0, 241,
            242, 5, 109, 0, 0, 242, 36, 1, 0, 0, 0, 243, 244, 5, 73, 0, 0, 244, 245, 5, 115, 0, 0, 245, 246,
            5, 90, 0, 0, 246, 247, 5, 101, 0, 0, 247, 248, 5, 114, 0, 0, 248, 249, 5, 111, 0, 0, 249, 38,
            1, 0, 0, 0, 250, 251, 5, 73, 0, 0, 251, 252, 5, 115, 0, 0, 252, 253, 5, 78, 0, 0, 253, 254, 5,
            111, 0, 0, 254, 255, 5, 110, 0, 0, 255, 256, 5, 90, 0, 0, 256, 257, 5, 101, 0, 0, 257, 258,
            5, 114, 0, 0, 258, 259, 5, 111, 0, 0, 259, 40, 1, 0, 0, 0, 260, 261, 5, 83, 0, 0, 261, 262, 5,
            84, 0, 0, 262, 263, 5, 82, 0, 0, 263, 264, 5, 73, 0, 0, 264, 265, 5, 78, 0, 0, 265, 273, 5, 71,
            0, 0, 266, 267, 5, 83, 0, 0, 267, 268, 5, 116, 0, 0, 268, 269, 5, 114, 0, 0, 269, 270, 5, 105,
            0, 0, 270, 271, 5, 110, 0, 0, 271, 273, 5, 103, 0, 0, 272, 260, 1, 0, 0, 0, 272, 266, 1, 0, 0,
            0, 273, 42, 1, 0, 0, 0, 274, 275, 5, 66, 0, 0, 275, 276, 5, 89, 0, 0, 276, 277, 5, 84, 0, 0, 277,
            278, 5, 69, 0, 0, 278, 279, 5, 65, 0, 0, 279, 280, 5, 82, 0, 0, 280, 281, 5, 82, 0, 0, 281, 282,
            5, 65, 0, 0, 282, 293, 5, 89, 0, 0, 283, 284, 5, 66, 0, 0, 284, 285, 5, 121, 0, 0, 285, 286,
            5, 116, 0, 0, 286, 287, 5, 101, 0, 0, 287, 288, 5, 97, 0, 0, 288, 289, 5, 114, 0, 0, 289, 290,
            5, 114, 0, 0, 290, 291, 5, 97, 0, 0, 291, 293, 5, 121, 0, 0, 292, 274, 1, 0, 0, 0, 292, 283,
            1, 0, 0, 0, 293, 44, 1, 0, 0, 0, 294, 295, 5, 73, 0, 0, 295, 296, 5, 78, 0, 0, 296, 297, 5, 84,
            0, 0, 297, 298, 5, 65, 0, 0, 298, 299, 5, 82, 0, 0, 299, 300, 5, 82, 0, 0, 300, 301, 5, 65, 0,
            0, 301, 311, 5, 89, 0, 0, 302, 303, 5, 73, 0, 0, 303, 304, 5, 110, 0, 0, 304, 305, 5, 116, 0,
            0, 305, 306, 5, 97, 0, 0, 306, 307, 5, 114, 0, 0, 307, 308, 5, 114, 0, 0, 308, 309, 5, 97, 0,
            0, 309, 311, 5, 121, 0, 0, 310, 294, 1, 0, 0, 0, 310, 302, 1, 0, 0, 0, 311, 46, 1, 0, 0, 0, 312,
            313, 5, 66, 0, 0, 313, 314, 5, 89, 0, 0, 314, 315, 5, 84, 0, 0, 315, 321, 5, 69, 0, 0, 316, 317,
            5, 66, 0, 0, 317, 318, 5, 121, 0, 0, 318, 319, 5, 116, 0, 0, 319, 321, 5, 101, 0, 0, 320, 312,
            1, 0, 0, 0, 320, 316, 1, 0, 0, 0, 321, 48, 1, 0, 0, 0, 322, 323, 5, 85, 0, 0, 323, 324, 5, 78,
            0, 0, 324, 325, 5, 83, 0, 0, 325, 326, 5, 73, 0, 0, 326, 327, 5, 71, 0, 0, 327, 328, 5, 78, 0,
            0, 328, 329, 5, 69, 0, 0, 329, 335, 5, 68, 0, 0, 330, 331, 5, 85, 0, 0, 331, 332, 5, 105, 0,
            0, 332, 333, 5, 110, 0, 0, 333, 335, 5, 116, 0, 0, 334, 322, 1, 0, 0, 0, 334, 330, 1, 0, 0, 0,
            335, 50, 1, 0, 0, 0, 336, 337, 5, 73, 0, 0, 337, 338, 5, 78, 0, 0, 338, 350, 5, 84, 0, 0, 339,
            340, 5, 73, 0, 0, 340, 341, 5, 110, 0, 0, 341, 350, 5, 116, 0, 0, 342, 343, 5, 73, 0, 0, 343,
            344, 5, 78, 0, 0, 344, 345, 5, 84, 0, 0, 345, 346, 5, 69, 0, 0, 346, 347, 5, 71, 0, 0, 347, 348,
            5, 69, 0, 0, 348, 350, 5, 82, 0, 0, 349, 336, 1, 0, 0, 0, 349, 339, 1, 0, 0, 0, 349, 342, 1, 0,
            0, 0, 350, 52, 1, 0, 0, 0, 351, 352, 5, 70, 0, 0, 352, 353, 5, 76, 0, 0, 353, 354, 5, 79, 0, 0,
            354, 355, 5, 65, 0, 0, 355, 362, 5, 84, 0, 0, 356, 357, 5, 70, 0, 0, 357, 358, 5, 108, 0, 0,
            358, 359, 5, 111, 0, 0, 359, 360, 5, 97, 0, 0, 360, 362, 5, 116, 0, 0, 361, 351, 1, 0, 0, 0,
            361, 356, 1, 0, 0, 0, 362, 54, 1, 0, 0, 0, 363, 364, 5, 83, 0, 0, 364, 365, 5, 69, 0, 0, 365,
            366, 5, 76, 0, 0, 366, 367, 5, 69, 0, 0, 367, 368, 5, 67, 0, 0, 368, 376, 5, 84, 0, 0, 369, 370,
            5, 115, 0, 0, 370, 371, 5, 101, 0, 0, 371, 372, 5, 108, 0, 0, 372, 373, 5, 101, 0, 0, 373, 374,
            5, 99, 0, 0, 374, 376, 5, 116, 0, 0, 375, 363, 1, 0, 0, 0, 375, 369, 1, 0, 0, 0, 376, 56, 1, 0,
            0, 0, 377, 378, 5, 83, 0, 0, 378, 379, 5, 84, 0, 0, 379, 380, 5, 82, 0, 0, 380, 381, 5, 69, 0,
            0, 381, 382, 5, 65, 0, 0, 382, 390, 5, 77, 0, 0, 383, 384, 5, 115, 0, 0, 384, 385, 5, 116, 0,
            0, 385, 386, 5, 114, 0, 0, 386, 387, 5, 101, 0, 0, 387, 388, 5, 97, 0, 0, 388, 390, 5, 109,
            0, 0, 389, 377, 1, 0, 0, 0, 389, 383, 1, 0, 0, 0, 390, 58, 1, 0, 0, 0, 391, 392, 5, 70, 0, 0, 392,
            393, 5, 82, 0, 0, 393, 394, 5, 79, 0, 0, 394, 400, 5, 77, 0, 0, 395, 396, 5, 102, 0, 0, 396,
            397, 5, 114, 0, 0, 397, 398, 5, 111, 0, 0, 398, 400, 5, 109, 0, 0, 399, 391, 1, 0, 0, 0, 399,
            395, 1, 0, 0, 0, 400, 60, 1, 0, 0, 0, 401, 402, 5, 68, 0, 0, 402, 403, 5, 69, 0, 0, 403, 404,
            5, 67, 0, 0, 404, 405, 5, 76, 0, 0, 405, 406, 5, 65, 0, 0, 406, 407, 5, 82, 0, 0, 407, 416, 5,
            69, 0, 0, 408, 409, 5, 100, 0, 0, 409, 410, 5, 101, 0, 0, 410, 411, 5, 99, 0, 0, 411, 412, 5,
            108, 0, 0, 412, 413, 5, 97, 0, 0, 413, 414, 5, 114, 0, 0, 414, 416, 5, 101, 0, 0, 415, 401,
            1, 0, 0, 0, 415, 408, 1, 0, 0, 0, 416, 62, 1, 0, 0, 0, 417, 418, 5, 70, 0, 0, 418, 419, 5, 73,
            0, 0, 419, 420, 5, 76, 0, 0, 420, 426, 5, 69, 0, 0, 421, 422, 5, 102, 0, 0, 422, 423, 5, 105,
            0, 0, 423, 424, 5, 108, 0, 0, 424, 426, 5, 101, 0, 0, 425, 417, 1, 0, 0, 0, 425, 421, 1, 0, 0,
            0, 426, 64, 1, 0, 0, 0, 427, 428, 5, 83, 0, 0, 428, 429, 5, 84, 0, 0, 429, 430, 5, 79, 0, 0, 430,
            431, 5, 82, 0, 0, 431, 432, 5, 65, 0, 0, 432, 433, 5, 71, 0, 0, 433, 442, 5, 69, 0, 0, 434, 435,
            5, 115, 0, 0, 435, 436, 5, 116, 0, 0, 436, 437, 5, 111, 0, 0, 437, 438, 5, 114, 0, 0, 438, 439,
            5, 97, 0, 0, 439, 440, 5, 103, 0, 0, 440, 442, 5, 101, 0, 0, 441, 427, 1, 0, 0, 0, 441, 434,
            1, 0, 0, 0, 442, 66, 1, 0, 0, 0, 443, 444, 5, 77, 0, 0, 444, 445, 5, 73, 0, 0, 445, 450, 5, 78,
            0, 0, 446, 447, 5, 109, 0, 0, 447, 448, 5, 105, 0, 0, 448, 450, 5, 110, 0, 0, 449, 443, 1, 0,
            0, 0, 449, 446, 1, 0, 0, 0, 450, 68, 1, 0, 0, 0, 451, 452, 5, 77, 0, 0, 452, 453, 5, 65, 0, 0,
            453, 458, 5, 88, 0, 0, 454, 455, 5, 109, 0, 0, 455, 456, 5, 97, 0, 0, 456, 458, 5, 120, 0, 0,
            457, 451, 1, 0, 0, 0, 457, 454, 1, 0, 0, 0, 458, 70, 1, 0, 0, 0, 459, 460, 5, 65, 0, 0, 460, 461,
            5, 86, 0, 0, 461, 466, 5, 71, 0, 0, 462, 463, 5, 97, 0, 0, 463, 464, 5, 118, 0, 0, 464, 466,
            5, 103, 0, 0, 465, 459, 1, 0, 0, 0, 465, 462, 1, 0, 0, 0, 466, 72, 1, 0, 0, 0, 467, 468, 5, 83,
            0, 0, 468, 469, 5, 85, 0, 0, 469, 470, 5, 77, 0, 0, 470, 476, 5, 67, 0, 0, 471, 472, 5, 115,
            0, 0, 472, 473, 5, 117, 0, 0, 473, 474, 5, 109, 0, 0, 474, 476, 5, 99, 0, 0, 475, 467, 1, 0,
            0, 0, 475, 471, 1, 0, 0, 0, 476, 74, 1, 0, 0, 0, 477, 481, 7, 0, 0, 0, 478, 480, 7, 1, 0, 0, 479,
            478, 1, 0, 0, 0, 480, 483, 1, 0, 0, 0, 481, 479, 1, 0, 0, 0, 481, 482, 1, 0, 0, 0, 482, 76, 1,
            0, 0, 0, 483, 481, 1, 0, 0, 0, 484, 490, 5, 39, 0, 0, 485, 489, 8, 2, 0, 0, 486, 487, 5, 39, 0,
            0, 487, 489, 5, 39, 0, 0, 488, 485, 1, 0, 0, 0, 488, 486, 1, 0, 0, 0, 489, 492, 1, 0, 0, 0, 490,
            488, 1, 0, 0, 0, 490, 491, 1, 0, 0, 0, 491, 493, 1, 0, 0, 0, 492, 490, 1, 0, 0, 0, 493, 494, 5,
            39, 0, 0, 494, 78, 1, 0, 0, 0, 495, 496, 3, 139, 69, 0, 496, 80, 1, 0, 0, 0, 497, 499, 3, 143,
            71, 0, 498, 497, 1, 0, 0, 0, 499, 500, 1, 0, 0, 0, 500, 498, 1, 0, 0, 0, 500, 501, 1, 0, 0, 0,
            501, 82, 1, 0, 0, 0, 502, 505, 3, 81, 40, 0, 503, 505, 3, 139, 69, 0, 504, 502, 1, 0, 0, 0, 504,
            503, 1, 0, 0, 0, 505, 506, 1, 0, 0, 0, 506, 508, 5, 69, 0, 0, 507, 509, 7, 3, 0, 0, 508, 507,
            1, 0, 0, 0, 508, 509, 1, 0, 0, 0, 509, 511, 1, 0, 0, 0, 510, 512, 3, 143, 71, 0, 511, 510, 1,
            0, 0, 0, 512, 513, 1, 0, 0, 0, 513, 511, 1, 0, 0, 0, 513, 514, 1, 0, 0, 0, 514, 84, 1, 0, 0, 0,
            515, 516, 5, 61, 0, 0, 516, 86, 1, 0, 0, 0, 517, 518, 5, 62, 0, 0, 518, 88, 1, 0, 0, 0, 519, 520,
            5, 60, 0, 0, 520, 90, 1, 0, 0, 0, 521, 522, 5, 33, 0, 0, 522, 92, 1, 0, 0, 0, 523, 524, 5, 124,
            0, 0, 524, 525, 5, 124, 0, 0, 525, 94, 1, 0, 0, 0, 526, 527, 5, 46, 0, 0, 527, 96, 1, 0, 0, 0,
            528, 529, 5, 95, 0, 0, 529, 98, 1, 0, 0, 0, 530, 531, 5, 64, 0, 0, 531, 100, 1, 0, 0, 0, 532,
            533, 5, 35, 0, 0, 533, 102, 1, 0, 0, 0, 534, 535, 5, 38, 0, 0, 535, 104, 1, 0, 0, 0, 536, 537,
            5, 37, 0, 0, 537, 106, 1, 0, 0, 0, 538, 539, 5, 36, 0, 0, 539, 108, 1, 0, 0, 0, 540, 541, 5, 44,
            0, 0, 541, 110, 1, 0, 0, 0, 542, 543, 5, 59, 0, 0, 543, 112, 1, 0, 0, 0, 544, 545, 5, 58, 0, 0,
            545, 114, 1, 0, 0, 0, 546, 547, 5, 58, 0, 0, 547, 548, 5, 58, 0, 0, 548, 116, 1, 0, 0, 0, 549,
            550, 5, 42, 0, 0, 550, 118, 1, 0, 0, 0, 551, 552, 5, 47, 0, 0, 552, 120, 1, 0, 0, 0, 553, 554,
            5, 43, 0, 0, 554, 122, 1, 0, 0, 0, 555, 556, 5, 45, 0, 0, 556, 124, 1, 0, 0, 0, 557, 558, 5, 126,
            0, 0, 558, 126, 1, 0, 0, 0, 559, 560, 5, 124, 0, 0, 560, 128, 1, 0, 0, 0, 561, 562, 5, 94, 0,
            0, 562, 130, 1, 0, 0, 0, 563, 565, 7, 4, 0, 0, 564, 563, 1, 0, 0, 0, 565, 566, 1, 0, 0, 0, 566,
            564, 1, 0, 0, 0, 566, 567, 1, 0, 0, 0, 567, 568, 1, 0, 0, 0, 568, 569, 6, 65, 0, 0, 569, 132,
            1, 0, 0, 0, 570, 571, 5, 47, 0, 0, 571, 572, 5, 42, 0, 0, 572, 577, 1, 0, 0, 0, 573, 576, 3, 133,
            66, 0, 574, 576, 9, 0, 0, 0, 575, 573, 1, 0, 0, 0, 575, 574, 1, 0, 0, 0, 576, 579, 1, 0, 0, 0,
            577, 578, 1, 0, 0, 0, 577, 575, 1, 0, 0, 0, 578, 580, 1, 0, 0, 0, 579, 577, 1, 0, 0, 0, 580, 581,
            5, 42, 0, 0, 581, 582, 5, 47, 0, 0, 582, 583, 1, 0, 0, 0, 583, 584, 6, 66, 1, 0, 584, 134, 1,
            0, 0, 0, 585, 586, 5, 35, 0, 0, 586, 587, 5, 32, 0, 0, 587, 591, 1, 0, 0, 0, 588, 590, 8, 5, 0,
            0, 589, 588, 1, 0, 0, 0, 590, 593, 1, 0, 0, 0, 591, 589, 1, 0, 0, 0, 591, 592, 1, 0, 0, 0, 592,
            594, 1, 0, 0, 0, 593, 591, 1, 0, 0, 0, 594, 595, 6, 67, 1, 0, 595, 136, 1, 0, 0, 0, 596, 597,
            7, 6, 0, 0, 597, 138, 1, 0, 0, 0, 598, 600, 3, 143, 71, 0, 599, 598, 1, 0, 0, 0, 600, 601, 1,
            0, 0, 0, 601, 599, 1, 0, 0, 0, 601, 602, 1, 0, 0, 0, 602, 603, 1, 0, 0, 0, 603, 605, 5, 46, 0,
            0, 604, 606, 3, 143, 71, 0, 605, 604, 1, 0, 0, 0, 606, 607, 1, 0, 0, 0, 607, 605, 1, 0, 0, 0,
            607, 608, 1, 0, 0, 0, 608, 623, 1, 0, 0, 0, 609, 611, 3, 143, 71, 0, 610, 609, 1, 0, 0, 0, 611,
            612, 1, 0, 0, 0, 612, 610, 1, 0, 0, 0, 612, 613, 1, 0, 0, 0, 613, 614, 1, 0, 0, 0, 614, 615, 5,
            46, 0, 0, 615, 623, 1, 0, 0, 0, 616, 618, 5, 46, 0, 0, 617, 619, 3, 143, 71, 0, 618, 617, 1,
            0, 0, 0, 619, 620, 1, 0, 0, 0, 620, 618, 1, 0, 0, 0, 620, 621, 1, 0, 0, 0, 621, 623, 1, 0, 0, 0,
            622, 599, 1, 0, 0, 0, 622, 610, 1, 0, 0, 0, 622, 616, 1, 0, 0, 0, 623, 140, 1, 0, 0, 0, 624, 625,
            7, 7, 0, 0, 625, 142, 1, 0, 0, 0, 626, 627, 7, 8, 0, 0, 627, 144, 1, 0, 0, 0, 34, 0, 272, 292,
            310, 320, 334, 349, 361, 375, 389, 399, 415, 425, 441, 449, 457, 465, 475, 481, 488, 490,
            500, 504, 508, 513, 566, 575, 577, 591, 601, 607, 612, 620, 622, 2, 6, 0, 0, 0, 1, 0
        };
        staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment,
                sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));
        antlr4::atn::ATNDeserializer deserializer;
        staticData->atn = deserializer.deserialize(staticData->serializedATN);
        const size_t count = staticData->atn->getNumberOfDecisions();
        staticData->decisionToDFA.reserve(count);
        for (size_t i = 0; i < count; i++)
            staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
        rqllexerLexerStaticData = staticData.release();
    }

}

RQLLexer::RQLLexer(CharStream* input) : Lexer(input)
{
    RQLLexer::initialize();
    _interpreter = new atn::LexerATNSimulator(this, *rqllexerLexerStaticData->atn, rqllexerLexerStaticData->decisionToDFA,
        rqllexerLexerStaticData->sharedContextCache);
}

RQLLexer::~RQLLexer()
{
    delete _interpreter;
}

std::string RQLLexer::getGrammarFileName() const
{
    return "RQL.g4";
}

const std::vector<std::string> &RQLLexer::getRuleNames() const
{
    return rqllexerLexerStaticData->ruleNames;
}

const std::vector<std::string> &RQLLexer::getChannelNames() const
{
    return rqllexerLexerStaticData->channelNames;
}

const std::vector<std::string> &RQLLexer::getModeNames() const
{
    return rqllexerLexerStaticData->modeNames;
}

const dfa::Vocabulary &RQLLexer::getVocabulary() const
{
    return rqllexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView RQLLexer::getSerializedATN() const
{
    return rqllexerLexerStaticData->serializedATN;
}

const atn::ATN &RQLLexer::getATN() const
{
    return *rqllexerLexerStaticData->atn;
}




void RQLLexer::initialize()
{
    std::call_once(rqllexerLexerOnceFlag, rqllexerLexerInitialize);
}
