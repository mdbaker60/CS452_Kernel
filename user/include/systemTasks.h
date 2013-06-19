#ifndef __SYSTEMTASKS_H__
#define __SYSTEMTASKS_H__

struct NotifierMessage {
  int type;
  int data;
};

void idleTask();
void notifier();

#endif
