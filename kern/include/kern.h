#ifndef __KERN_H__
#define __KERN_H__

#include <values.h>
#include <task.h>

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) > (Y)) ? (Y) : (X))

#define MAXTASKS 100

int *getSP();
void setIRQ_SP(int SP);
void enableCache();
int Create_sys(int priority, void (*code)());
void syscall_enter();
void int_enter();
void getNextRequest(struct Task *task, struct Request *request);
void handle(struct Request *request);
struct Task *getNextTask();
void makeTaskReady(struct Task *task);

#endif
