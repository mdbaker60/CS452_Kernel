#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <ts7200.h>

static int *clockValue = (int *)(TIMER3_BASE + VAL_OFFSET);

void clockInit();
int getClockTick();

#endif
