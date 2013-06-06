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

void selfDestroy() {
  Destroy(MyTid());
}

void testerChild() {
  char reply[100];
  int src;

  bwprintf(COM2, "CHILD: My TID is %d and my parent's TID is %d\r", MyTid(), MyParentTid());

  Receive(&src, reply, 100);
  bwprintf(COM2, "CHILD: received message \"%s\"\r", reply);
  Reply(src, "Thanks", 6);

  Send(MyParentTid(), "Message to tester", 18, reply, 100);
  bwprintf(COM2, "CHILD: received reply \"%s\"\r", reply);

  Destroy(MyTid());
}

void tester() {
  char reply[100];
  int src;

  int child = Create(1, testerChild);
  bwprintf(COM2, "TESTER:My TID is %d and my child's TID is %d\r", MyTid(), child);
  Pass();
  
  int err = Send(child, "Message from tester", 20, reply, 100);
  bwprintf(COM2, "TESTER: received reply \"%s\"\r", reply);

  Receive(&src, reply, 100);
  bwprintf(COM2, "TESTER: received message \"%s\"\r", reply);
  Reply(src, "Got the message", 16);

  Destroy(MyTid());
}

void firstTask() {
  Create(0, busyTask);

  int i;
  for(i=0; i<98; i++) {
    Create(7, selfDestroy);
  }
  
  Create(2, tester);

  Destroy(MyTid());
}
