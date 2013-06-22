#ifndef __SYSTEMTASKS_H__
#define __SYSTEMTASKS_H__

struct NotifierMessage {
  int type;
  int data;
};

struct NotifierMessageBuf {
  int type;
  int data[17];
};
void idleTask();
void notifier();
void bufferedNotifier(); 
#endif
