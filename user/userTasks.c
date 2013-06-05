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
  Send(MyParentTid(), (char *)&msg, sizeof(int), (char *)&reply, sizeof(int));
  Exit();
}

void firstTask() {
  Create(0, busyTask);

  int msg, reply, tid, i;

  int ids[98];

  for(i=0; i<98; i++) {
    ids[i] = Create(1, tester);
    bwprintf(COM2, "Created task %d\r", ids[i]);
  }
  bwgetc(COM2);
  for(i=0; i<98; i++) {
    Receive(&tid, (char *)&msg, sizeof(int));
  }
  for(i=0; i<98; i++) {
    Destroy(ids[i]);
  }

  ids[0] = Create(1, tester);
  ids[1] = Create(1, tester);

  Receive(&tid, (char *)&msg, sizeof(int));
  Receive(&tid, (char *)&msg, sizeof(int));
  Reply(ids[0], (char *)&reply, sizeof(int));
  Reply(ids[1], (char *)&reply, sizeof(int));
  Exit();
}
