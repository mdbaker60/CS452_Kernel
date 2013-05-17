#ifndef __TASK_H__
#define __TASK_H__

struct Task {
  //the saved values of the 16 registers
  int R0;
  int R1;
  int R2;
  int R3;
  int R4;
  int R5;
  int R6;
  int R7;
  int R8;
  int R9;
  int R10;
  int FP;
  int SP;
  int LR;
  int PC;
};

#endif
