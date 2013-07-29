#ifndef __ASSERT_H__
#define __ASSERT_H__

#include <bwio.h>

#define assert(CHECK, ...) do {\
if(!(CHECK)) {\
bwprintf(COM2, "\e[1;1f\e[2JAssertion Failure:");\
bwprintf(COM2,__VA_ARGS__);\
bwprintf(COM2, "\r");\
Shutdown();\
}\
}while(0)

#endif
