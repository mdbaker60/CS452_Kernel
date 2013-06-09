#include <userTasks.h>
#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>
#include <clockServer.h>
#include <values.h>

void idleTask() {
  while(true) Pass();
}

struct NotifierMessage {
  int type;
  int data;
};

void notifier() {
  int server, eventType, reply = 0;
  struct NotifierMessage msg;
  msg.type = 0;
  Receive(&server, (char *)&eventType, sizeof(int));
  Reply(server, (char *)&reply, sizeof(int));
  while(true) {
    msg.data = AwaitEvent(eventType);
    Send(server, (char *)&msg, sizeof(struct NotifierMessage), (char *)&reply, sizeof(int));
  }
}

struct msg{
  int delayTime;
  int delayNum;
};
void clientTask(){
  int parent = MyParentTid();
  int tid = MyTid();
  struct msg params;
  int i = 1;
  Send(parent, (char *)&i, sizeof(int), (char *)&params, sizeof(struct msg));
  while(params.delayNum >= i){
    Delay(params.delayTime);
    bwprintf(COM2, "Task %d: Delay Time %d, Finished delay %d\r", tid, params.delayTime, i);
    i++; 
  }

  notifyFirstTaskDone();
  tid = MyTid();
  Destroy(MyTid());
}

void firstTask() {
  int src, in, numTasks = 4;
  struct msg buf;
  Create(6, NSInit);
  Create(6, CSInit);
  int idleTid = Create(0, idleTask);
  int one = Create(4, clientTask);
  int two = Create(3, clientTask);
  int three = Create(2, clientTask);
  int four = Create(1, clientTask);

  Receive(&src, (char *)&in, sizeof(int));
  Receive(&src, (char *)&in, sizeof(int));
  Receive(&src, (char *)&in, sizeof(int));
  Receive(&src, (char *)&in, sizeof(int));

  buf.delayTime = 10;
  buf.delayNum = 20;
  Reply(one, (char *)&buf, sizeof(struct msg));
  buf.delayTime = 23;
  buf.delayNum = 9;
  Reply(two, (char *)&buf, sizeof(struct msg));
  buf.delayTime = 33;
  buf.delayNum = 6;
  Reply(three, (char *)&buf, sizeof(struct msg));
  buf.delayTime = 71;
  buf.delayNum = 3;
  Reply(four, (char *)&buf, sizeof(struct msg));

  //wait until all tasks have notified as being done, then shut down servers and exit
  int diff, garbage;
  while(numTasks > 0) {
    Receive(&src, (char *)&diff, sizeof(int));
    numTasks += diff;
    Destroy(src);
  }
  //shutdown clock server
  diff = CSSHUTDOWN;
  src = whoIs("Clock Server");
  Send(src, (char *)&diff, sizeof(int), (char *)&garbage, sizeof(int));
  //shut down name server
  diff = NSSHUTDOWN;
  Send(1, (char *)&diff, sizeof(int), (char *)&garbage, sizeof(int));
  //destroy idle task
  Destroy(idleTid);

  Destroy(MyTid());
}

void notifyFirstTaskDone() {
  int msg = -1, reply = 0;
  Send(0, (char *)&msg, sizeof(int), (char *)reply, sizeof(int));
}
