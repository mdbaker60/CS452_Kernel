#include <clockServer.h>
#include <nameServer.h>
#include <syscall.h>
#include <values.h>

struct WaitingTask {
  int ID;
  int ticksLeft;
  struct WaitingTask *next;
  struct WaitingTask *last;
};

struct Message {
  int type;
  int tid;
  int ticks;
};

void ClockHelper() {
  int clockServer = MyParentTid();
  struct Message msg;
  int reply;
  msg.type = 0;			//clock ticked
  while(true) {
    AwaitEvent(0);
    Send(clockServer, (char *)&msg, sizeof(struct Message), (char *)&reply, sizeof(int));
  }
}

void CSInit() {
  struct Message msg;
  int reply;
  int src;
  int ticks = 0;

  struct WaitingTask waitingTasks[100];
  struct WaitingTask *head = NULL;
  int i;
  for(i=0; i<100; i++) {
    waitingTasks[i].ID = i;
  }

  Create(7, ClockHelper);
  RegisterAs("Clock Server");
  
  struct WaitingTask *tempTask;
  while(true) {
    Receive(&src, (char *)&msg, sizeof(struct Message));
    switch(msg.type) {
      case 0:	//clock ticked
	++ticks;
	Reply(src, (char *)&reply, sizeof(int));
	tempTask = head;
	while(tempTask != NULL) {
	  --(tempTask->ticksLeft);
	  if(tempTask->ticksLeft == 0) {
	    reply = 0;
	    Reply(tempTask->ID, (char *)&reply, sizeof(int));
	    if(tempTask->last == NULL && tempTask->next == NULL) {
	      head = NULL;
	    }else if(tempTask->last == NULL) {
	      head = tempTask->next;
	      (tempTask->next)->last = NULL;
	    }else if(tempTask->next == NULL) {
	      (tempTask->last)->next = NULL;
	    }else{
	      (tempTask->last)->next = tempTask->next;
	      (tempTask->next)->last = tempTask->last;
	    }
	  }
	  tempTask = tempTask->next;
	}
	break;
      case 1:
	msg.ticks -= ticks;
      case 2:	//add waiting task
	if(msg.ticks <= 0) {
	  reply = 0;
	  Reply(src, (char *)&reply, sizeof(int));
 	}else{
	  tempTask = &waitingTasks[msg.tid];
	  tempTask->ticksLeft = msg.ticks;
	  tempTask->next = head;
	  tempTask->last = NULL;
	  head = tempTask;
 	}
	break;
      case 3:	//get time
	Reply(src, (char *)&ticks, sizeof(int));
	break;
    }
  }
}

int Delay(int ticks) {
  struct Message msg;
  int reply;
  int clockServer = whoIs("Clock Server");
  msg.type = 2;
  msg.tid = MyTid();
  msg.ticks = ticks;
  Send(clockServer, (char *)&msg, sizeof(struct Message), (char *)&reply, sizeof(int));
  return reply;
}

int Time() {
  struct Message msg;
  int reply;
  int clockServer = whoIs("Clock Server");
  msg.type = 3;
  Send(clockServer, (char *)&msg, sizeof(struct Message), (char *)&reply, sizeof(int));
  return reply;
}

int DelayUntil(int ticks) {
  struct Message msg;
  int reply;
  int clockServer = whoIs("Clock Server");
  msg.type = 1;
  msg.tid = MyTid();
  msg.ticks = ticks;
  Send(clockServer, (char *)&msg, sizeof(struct Message), (char *)&reply, sizeof(int));
  return reply;
}
