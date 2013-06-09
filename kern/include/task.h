#ifndef __TASK_H__
#define __TASK_H__

enum State {READY, ACTIVE, ZOMBIE, RCV_BL, RPL_BL, SND_BL, EVT_BL};

#define INDEX_MASK 0x7F
#define GEN_UNIT 0x80

struct Task {
  int *SP;
  int SPSR;
  int returnAddress;
  int ID;
  int parentID;
  struct Task *next;
  struct Task *last;
  enum State state;
  int priority;
  char *messageBuffer;
  int messageLength;
  char *replyBuffer;
  int replyLength;
  int *senderTid;
  int receiverTid;	//for destroying RCV_BL tasks
  struct Task *sendQHead;
  struct Task *sendQTail;
  int totalTime;
  int event;
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
