#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>
#include <clockServer.h>
#include <values.h>

void busyTask() {
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
  bwprintf(COM2, "new client task executing\r");
  int parent = MyParentTid();
  bwprintf(COM2, "obtained parent's tid\r");
  int tid = MyTid();
  bwprintf(COM2, "obtained my tid\r");
  struct msg params;
  int i = 1;
  //Priority
  //Delay time
  //Number of delays
  //Send a message to the first task which returns CS parameters
  bwprintf(COM2, "sending message to parent(task %d)\r", parent);
  Send(parent, (char *)&i, sizeof(int), (char *)&params, sizeof(struct msg));
  bwprintf(COM2, "%d: Delay Time %d with %d delays\r", tid, params.delayTime, params.delayNum);
  while(params.delayNum >= i){
    Delay(params.delayTime);
    bwprintf(COM2, "%d: Delay Time: %d, Finished delay: %d\r", tid, params.delayTime, i);
    i++; 
  }	
  Exit();
}

void firstTask() {
  int src, in;
  struct msg buf;
  Create(7, NSInit);
  Create(7, CSInit);
  int one = Create(4, clientTask);
  int two = Create(3, clientTask);
  int three = Create(2, clientTask);
  int four = Create(1, clientTask);
  Create(0, busyTask);

  bwprintf(COM2, "waiting for messages from all tasks(%d, %d, %d, %d)\r", one, two, three, four);
  Receive(&src, (char *)&in, sizeof(int));
  bwprintf(COM2, "got 1 message\r");
  Receive(&src, (char *)&in, sizeof(int));
  bwprintf(COM2, "got 2 messages\r");
  Receive(&src, (char *)&in, sizeof(int));
  bwprintf(COM2, "got 3 messages\r");
  Receive(&src, (char *)&in, sizeof(int));
  bwprintf(COM2, "got 4 messages\r");

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

  Exit();
}
