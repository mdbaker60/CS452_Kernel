#ifndef __SYSTEMTASKS_H__
#define __SYSTEMTASKS_H__

struct NotifierMessage {
  int type;
  int data;
};

struct NotifierMessageBuf {
  int type;
  char data[64];
};
void idleTask();
void notifier();
void bufferedNotifier(); 
#endif
