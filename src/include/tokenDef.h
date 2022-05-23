#include <string>

// Based on https://www.codeproject.com/Articles/10500/Converting-C-enums-to-strings

#ifndef ENUM_DEFINED

#ifndef BEGIN
#define BEGIN(ENUM_NAME) enum ENUM_NAME
#define DECL(element) element
#define END(ENUM_NAME) \
    ;                  \
    std::string GetString##ENUM_NAME(enum ENUM_NAME index);
#endif

BEGIN(command_id)
{
    DECL(VOID_COMMAND),
         DECL(VOID_VALUE),
         DECL(PUSH_ID),
         DECL(PUSH_ID1),
         DECL(PUSH_ID2),
         DECL(PUSH_ID3),
         DECL(PUSH_ID4),
         DECL(PUSH_ID5),
         DECL(PUSH_IDX),
         DECL(PUSH_VAL),
         DECL(PUSH_TSCAN),
         DECL(TYPE),
         DECL(ADD),
         DECL(SUBTRACT),
         DECL(MULTIPLY),
         DECL(DIVIDE),
         DECL(NEGATE),
         DECL(AND),
         DECL(OR),
         DECL(NOT),
         DECL(CMP_EQUAL),
         DECL(CMP_LT),
         DECL(CMP_GT),
         DECL(CMP_LE),
         DECL(CMP_GE),
         DECL(CMP_NOT_EQUAL),
         DECL(STREAM_AVG),
         DECL(STREAM_MIN),
         DECL(STREAM_MAX),
         DECL(STREAM_SUM),
         DECL(CALL),
         DECL(AGSE),
         DECL(PUSH_STREAM),
         DECL(STREAM_HASH),
         DECL(STREAM_DEHASH_DIV),
         DECL(STREAM_DEHASH_MOD),
         DECL(STREAM_ADD),
         DECL(STREAM_SUBSTRACT),
         DECL(STREAM_TIMEMOVE),
         DECL(STREAM_AGSE),
         DECL(COUNT),
         DECL(COUNT_RANGE)
}
END(command_id)

#undef BEGIN
#undef END
#undef DECL
#define ENUM_DEFINED
#endif