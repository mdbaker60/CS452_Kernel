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

void firstTask() {
  bwprintf(COM2, "First user task starting\r");
  Create(0, idleTask);
  bwprintf(COM2, "created idle task\r");
  char c = ' ';
  while(c != 'q') {
    bwprintf(COM2, "waiting on terminal input\r");
    c = (char)AwaitEvent(TERMIN_EVENT);
    bwprintf(COM2, "received terminal input\r");
    bwputc(COM2, c);
  }

  Shutdown();
}
