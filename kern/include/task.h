#ifndef __TASK_H__
#define __TASK_H__

enum State {READY, ACTIVE, ZOMBIE};

struct Task {
  int *SP;
  int SPSR;
  int returnAddress;
  int ID;
  int parentID;
  struct Task *next;
  enum State state;
  int priority;
};

struct Request {
  int ID;
  int arg1;
  int arg2;
  int arg3;
  int arg4;
  int arg5;
};

#endif
