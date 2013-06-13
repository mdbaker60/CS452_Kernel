#include <userTasks.h>
#include <clockServer.h>
#include <nameServer.h>
#include <syscall.h>
#include <values.h>
#include <bwio.h>

struct WaitingTask {
  int ID;
  int untilTick;
  struct WaitingTask *next;
  //struct WaitingTask *last;
};

struct CSMessage {
  int type;
  int tid;
  int ticks;
};

void CSInit() {
  struct CSMessage msg;
  int reply;
  int src;
  int ticks = 0;

  struct WaitingTask waitingTasks[100];
  struct WaitingTask *head = NULL;
  struct WaitingTask *last = NULL;
  int i;
  for(i=0; i<100; i++) {
    waitingTasks[i].ID = i;
  }

  int notifierTid = Create(7, notifier);
  int eventType = CLOCK_EVENT;
  Send(notifierTid, (char *)&eventType, sizeof(int), (char *)&reply, sizeof(int));
  RegisterAs("Clock Server");
  
  struct WaitingTask *tempTask;
  while(true) {
    Receive(&src, (char *)&msg, sizeof(struct CSMessage));
    switch(msg.type) {
      case CSNOTIFIER:		//from notifier
	++ticks;
	Reply(src, (char *)&reply, sizeof(int));
	tempTask = head;
	while(tempTask != NULL && tempTask->untilTick <= ticks) {
	  reply = 0;
	  Reply(tempTask->ID, (char *)&reply, sizeof(int));
	  
	  head = tempTask->next;
	  tempTask->next = NULL;
	  tempTask = head;
	} 
	break;
      case CSDELAY:
	msg.ticks += ticks;
      case CSDELAYUNTIL:	//add waiting task	
	if(msg.ticks <= ticks) {
	  reply = 0;
	  Reply(src, (char *)&reply, sizeof(int));
 	}else{
	  tempTask = &waitingTasks[msg.tid & 0x7F];
	  tempTask->ID = msg.tid;
	  tempTask->untilTick = msg.ticks;
	  tempTask->next = head;
	  while (tempTask->next != NULL && tempTask->next->untilTick < tempTask->untilTick){
		if (last != NULL) last->next = tempTask->next;
		last = tempTask->next;
	  	tempTask->next = tempTask->next->next;
	  }
	  if (tempTask->next == head){
		head = tempTask;
	  }else{
		last->next = tempTask;
		last = NULL;
	  }
 	}
	break;
      case CSGETTIME:	//get time
	Reply(src, (char *)&ticks, sizeof(int));
	break;
    }
  }
}

int Delay(int ticks) {
  struct CSMessage msg;
  int reply;
  int clockServer = whoIs("Clock Server");
  msg.type = CSDELAY;
  msg.tid = MyTid();
  msg.ticks = ticks;
  Send(clockServer, (char *)&msg, sizeof(struct CSMessage), (char *)&reply, sizeof(int));
  
  return reply;
}

int Time() {
  struct CSMessage msg;
  int reply;
  int clockServer = whoIs("Clock Server");
  msg.type = CSGETTIME;
  Send(clockServer, (char *)&msg, sizeof(struct CSMessage), (char *)&reply, sizeof(int));
  return reply;
}

int DelayUntil(int ticks) {
  struct CSMessage msg;
  int reply;
  int clockServer = whoIs("Clock Server");
  msg.type = CSDELAYUNTIL;
  msg.tid = MyTid();
  msg.ticks = ticks;
  Send(clockServer, (char *)&msg, sizeof(struct CSMessage), (char *)&reply, sizeof(int));
  return reply;
}
