#include <kern.h>
#include <bwio.h>
#include <task.h>
#include <syscall.h>
#include <userTasks.h>
#include <queue.h>

static int *freeMemStart;
static int *kernMemStart;
static int nextTID;
static struct Task *active;
static struct PriorityQueue *readyQueue;
static struct Request *activeRequest;
static struct Task *taskArray;

//TODO remove
void memcopy(char *destination, const char *source, int num);

int main() {
  struct Request kactiveRequest;
  struct PriorityQueue kreadyQueue;
  struct Task ktaskArray[MAXTASKS];
  activeRequest = &kactiveRequest; 
  readyQueue = &kreadyQueue;
  taskArray = ktaskArray;

  *((int *)0x28) = (int)syscall_enter;
  nextTID = 0;
  kernMemStart = getSP();
  //leave 250KB of stack space for function calls
  kernMemStart -= 0xFA00;

  initQueue(readyQueue);
  freeMemStart = kernMemStart;

  //create the first user task
  active = NULL;
  Create_sys(1, firstTask);
  active = dequeue(readyQueue);
  active->state = ACTIVE;
  while(active != NULL) {
    getNextRequest(active, activeRequest);
    handle(activeRequest);
  }

  int i;
  for(i=0; i<nextTID; i++) {
    if(taskArray[i].state != ZOMBIE) {
      bwprintf(COM2, "WARNING: task %d has not exited\r", i);
    }
  }

  return 0;
}

int Create_sys(int priority, void (*code)()) {
  struct Task *newTD = &taskArray[nextTID];
  //initialize user task context
  newTD->SP = freeMemStart-56;		//loaded during user task schedule
  newTD->ID = nextTID++;
  newTD->SPSR = 0xD0;
  if(active == NULL) {
    newTD->parentID = -1;
  }else{
    newTD->parentID = active->ID;
  }
  newTD->returnAddress = (int)code;
  newTD->state = READY;
  newTD->priority = priority;
  newTD->next = NULL;
  newTD->sendQHead = NULL;
  *(newTD->SP + 12) = (int)freeMemStart;	//stack pointer
  enqueue(readyQueue, newTD, priority);

  freeMemStart -= 0xFA00;	//give each task 250KB of stack space

  return newTD->ID;
}

void handle(struct Request *request) {
  switch(request->ID) {
    case 0:					//Create
      if((int *)request->arg2 >= freeMemStart) {	//code pointer on the stack
        *(active->SP) = -3;
      }else if(request->arg1 >= NUMPRIO || request->arg1 < 0) {	//invalid priority
	*(active->SP) = -1;
      }else if(nextTID >= MAXTASKS) {				//out of TDs
        *(active->SP) = -2;
      }else{
        *(active->SP) = Create_sys(request->arg1, (void (*)())request->arg2);
      }
      active->state = READY;
      enqueue(readyQueue, active, active->priority);
      active = dequeue(readyQueue);
      active->state = ACTIVE;
      break;
    case 1:				//MyTid
      *(active->SP) = active->ID;
      active->state = READY;
      enqueue(readyQueue, active, active->priority);
      active = dequeue(readyQueue);
      active->state = ACTIVE;
      break;
    case 2:				//MyParentTid
      *(active->SP) = active->parentID;
      active->state = READY;
      enqueue(readyQueue, active, active->priority);
      active = dequeue(readyQueue);
      active->state = ACTIVE;
      break;
    case 3:				//Pass
      active->state = READY;
      enqueue(readyQueue, active, active->priority);
      active = dequeue(readyQueue);
      active->state = ACTIVE;
      break;
    case 4:				//Exit
      active->state = ZOMBIE;
      active = dequeue(readyQueue);
      active->state = ACTIVE;
      break;
    case 5:				//Send
      if(taskArray[request->arg1].state == SND_BL) {
        struct Task *receiverTask = &taskArray[request->arg1];
	int messageLength = (receiverTask->messageLength < request->arg3) 
		? receiverTask->messageLength : request->arg3;
        memcopy(receiverTask->messageBuffer, (char *)request->arg2, messageLength);
	*(receiverTask->senderTid) = active->ID;
	*(receiverTask->SP) = request->arg3;
	*(active->SP) = 0;
        receiverTask->state = READY;
        enqueue(readyQueue, receiverTask, receiverTask->priority);
        active->state = RPL_BL;
	active->replyBuffer = (char *)request->arg4;
	active->replyLength = request->arg5;
        active = dequeue(readyQueue);
        active->state = ACTIVE;
      }else{
	struct Task *receiverTask = &taskArray[request->arg1];
	active->state = RCV_BL;
	active->messageBuffer = (char *)request->arg2;
	active->messageLength = request->arg3;
	active->replyBuffer = (char *)request->arg4;
	active->replyLength = request->arg5;
	if(receiverTask->sendQHead == NULL) {
	  receiverTask->sendQHead = receiverTask->sendQTail = active;
	}else{
	  (receiverTask->sendQTail)->next = active;
	  receiverTask->sendQTail = active;
	}
	active = dequeue(readyQueue);
	active->state = ACTIVE;
      }
      break;
    case 6:				//Receive
      if(active->sendQHead == NULL) {	//No tasks have sent
        active->state = SND_BL;
        active->messageBuffer = (char *)request->arg2;
        active->messageLength = request->arg3;
        active->senderTid = (int *)request->arg1;
        active = dequeue(readyQueue);
        active->state = ACTIVE;
      }else{
	struct Task *senderTask = active->sendQHead;
	active->sendQHead = senderTask->next;
	senderTask->next = NULL;
	int messageLength = (senderTask->messageLength < request->arg3)
		? senderTask->messageLength : request->arg3;
	memcopy((char *)request->arg2, senderTask->messageBuffer, messageLength);
	*((int *)request->arg1) = senderTask->ID;
	*(active->SP) = senderTask->messageLength;
	*(senderTask->SP) = 0;
	senderTask->state = RPL_BL;
	active->state = READY;
	enqueue(readyQueue, active, active->priority);
	active = dequeue(readyQueue);
	active->state = ACTIVE;
      }
      break;
    case 7:				//Reply
      if(taskArray[request->arg1].state == RPL_BL) {
	struct Task *senderTask = &taskArray[request->arg1];
	int replyLength = (senderTask->replyLength < request->arg3)
		? senderTask->replyLength : request->arg3;
	memcopy(senderTask->replyBuffer, (char *)request->arg2, replyLength);
	*(senderTask->SP) = request->arg3;
	*(active->SP) = 0;
	senderTask->state = READY;
	enqueue(readyQueue, senderTask, senderTask->priority);
	active->state = READY;
	enqueue(readyQueue, active, active->priority);
	active = dequeue(readyQueue);
	active->state = ACTIVE;
      }else{
	bwprintf(COM2, "task not reply blocked\r");
      }
      break;
  }
  //TODO move scheduling into functions
}

//TODO move this somewhere better
void memcopy(char *destination, const char *source, int num) {
  int i;
  for(i=0; i<num; i++) {
    *destination++ = *source++;
  }
}
