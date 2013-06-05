#ifndef __CLOCKSERVER_H__
#define __CLOCKSERVER_H__

#define NOTIFIER	0
#define DELAYUNTIL	1
#define DELAY		2
#define GETTIME		3

void CSInit();
int Delay(int ticks);
int Time();
int DelayUntil(int ticks);

#endif
