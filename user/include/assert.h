#ifndef __ASSERT_H__
#define __ASSERT_H__

#include <bwio.h>

#define assert(CHECK, MSG) do {\
if(!(CHECK)) {\
bwprintf(COM2, "\e[1;1f\e[2JAssertion Failure: %s\r", MSG);\
Shutdown();\
}\
}while(0)

#endif
