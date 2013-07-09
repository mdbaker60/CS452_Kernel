#include <systemTasks.h>
#include <syscall.h>
#include <values.h>

#include <bwio.h>

void idleTask() {
  while(true) Pass();
}

void bufferedNotifier(){
  int server, err, eventType, reply = 0;
  struct NotifierMessageBuf msg;
  msg.type = 0;
  Receive(&server, (char *)&eventType, sizeof(int));
  Reply(server, (char *)&reply, sizeof(int));

  while(true){
    err = AwaitEvent(eventType, (char *)msg.data, 17);
    Send(server, (char *)&msg, sizeof(struct NotifierMessageBuf), (char *)&reply, sizeof(int));
  }
}
void notifier() {
  int server, eventType, reply = 0;
  struct NotifierMessage msg;
  msg.type = 0;
  Receive(&server, (char *)&eventType, sizeof(int));
  Reply(server, (char *)&reply, sizeof(int));
  while(true) {
    msg.data = AwaitEvent(eventType, NULL, 0);
    Send(server, (char *)&msg, sizeof(struct NotifierMessage), (char *)&reply, sizeof(int));
  }
}
