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

void tester() {
  bwprintf(COM2, "My TID is %d\r", MyTid());
  int msg, reply;
  //Send(MyParentTid(), (char *)&msg, sizeof(int), (char *)&reply, sizeof(int));
  Exit();
}

void firstTask() {
  Create(0, busyTask);

  int msg, tid, i;

  int child, ids[98];

  for(i=0; i<10; i++) {
    ids[i] = Create(1, tester);
  }
  //for(i=0; i<10; i++) {
    //Receive(&tid, (char *)&msg, sizeof(int));
  //}
  //for(i=0; i<10; i++) {
    //Reply(ids[i], (char *)&msg, sizeof(int));
    //Destroy(ids[i]);
  //}

  Exit();
}
