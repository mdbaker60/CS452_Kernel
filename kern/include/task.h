#ifndef __TASK_H__
#define __TASK_H__

struct Task {
  int *SP;
  int SPSR;
  int ID;
  struct Task *parent;
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
