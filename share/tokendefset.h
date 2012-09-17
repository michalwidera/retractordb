/*Copyright 2010-2012 Streams Lab Michal Widera

  RESTRICTED RIGHTS LEGEND
  Use, duplication or disclosure by any commercial industry (public or
  private), private individual or by any Government Agency without an
  expressed written consent of release from Streams Lab Michal Widera
  is subject to restriction of the Rights.

  Manufacturer is:
      Streams Lab Michal Widera
      Reymonta str. 56/1
      Poland, Zabrze, 41-800
      (+48) 606 995 918
*/
#ifdef DEF_CASE
DEF_CASE( VOID_COMMAND      ) 
DEF_CASE( VOID_VALUE        ) 
DEF_CASE( PUSH_ID           ) 
DEF_CASE( PUSH_ID1          ) 
DEF_CASE( PUSH_ID2          ) 
DEF_CASE( PUSH_ID3          ) 
DEF_CASE( PUSH_ID4          ) 
DEF_CASE( PUSH_IDX          ) 
DEF_CASE( PUSH_VAL          ) 
DEF_CASE( PUSH_TSCAN        ) 
DEF_CASE( TYPE              ) 
DEF_CASE( ADD               ) 
DEF_CASE( SUBTRACT          ) 
DEF_CASE( MULTIPLY          ) 
DEF_CASE( DIVIDE            ) 
DEF_CASE( NEGATE            ) 
DEF_CASE( AND               ) 
DEF_CASE( OR                ) 
DEF_CASE( NOT               ) 
DEF_CASE( CMP_EQUAL         ) 
DEF_CASE( CMP_LT            ) 
DEF_CASE( CMP_GT            ) 
DEF_CASE( CMP_LE            ) 
DEF_CASE( CMP_GE            ) 
DEF_CASE( CMP_NOT_EQUAL     ) 
DEF_CASE( AVG               ) 
DEF_CASE( MIN               ) 
DEF_CASE( MAX               ) 
DEF_CASE( SUM               ) 
DEF_CASE( CALL              ) 
DEF_CASE( AGSE              ) 
DEF_CASE( PUSH_STREAM       ) 
DEF_CASE( STREAM_HASH       ) 
DEF_CASE( STREAM_DEHASH_DIV ) 
DEF_CASE( STREAM_DEHASH_MOD ) 
DEF_CASE( STREAM_ADD        ) 
DEF_CASE( STREAM_SUBSTRACT  ) 
DEF_CASE( STREAM_TIMEMOVE   ) 
DEF_CASE( STREAM_AGSE       ) 
#else
#error Wymagane jest zdefiniowanie operacji DEF_CASE
#endif
