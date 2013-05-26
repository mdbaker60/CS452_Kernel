#ifndef __KERN_H__
#define __KERN_H__

#include <values.h>
#include <task.h>

#define MAXTASKS 100 //TODO CHANGE ME BACK

int *getSP();
int Create_sys(int priority, void (*code)());
void syscall_enter();
void getNextRequest(struct Task *task, struct Request *request);
void handle(struct Request *request);

#endif
