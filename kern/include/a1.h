#ifndef __A1_H__
#define __A1_H__

#include <values.h>
#include <task.h>

#define MAXTASKS 512

int *getSP();
int Create_sys(int priority, void (*code)());
void syscall_enter();
void getNextRequest(struct Task *task, struct Request *request);
void handle(struct Request *request);

#endif
