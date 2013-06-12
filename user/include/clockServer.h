#ifndef __CLOCKSERVER_H__
#define __CLOCKSERVER_H__

#define CSNOTIFIER	0
#define CSDELAYUNTIL	1
#define CSDELAY		2
#define CSGETTIME	3

void CSInit();
int Delay(int ticks);
int Time();
int DelayUntil(int ticks);

#endif
