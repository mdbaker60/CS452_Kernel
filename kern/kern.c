#include <kern.h>
#include <bwio.h>
#include <task.h>
#include <syscall.h>
#include <userTasks.h>
#include <queue.h>
#include <mem.h>

//TODO remove
#include <ts7200.h>
static int *clockClear;

static int *freeMemStart;
static int *kernMemStart;
static int nextTID;
static struct Task *active;
static struct PriorityQueue *readyQueue;
static struct Request *activeRequest;
static struct Task *taskArray;

void print() {
  bwprintf(COM2, "test\r");
}

int main() {
  struct Request kactiveRequest;
  struct PriorityQueue kreadyQueue;
  struct Task ktaskArray[MAXTASKS];
  activeRequest = &kactiveRequest; 
  readyQueue = &kreadyQueue;
  taskArray = ktaskArray;

  //TODO remove
  int *clockControl = (int *)(TIMER3_BASE + CRTL_OFFSET);
  int *clockLoad = (int *)(TIMER3_BASE + LDR_OFFSET);
  clockClear = (int *)(TIMER3_BASE + CLR_OFFSET);
  int *intControl = (int *)0x800C0010;
  *intControl = 0x80000;
  *clockLoad = 507999;
  *clockControl = ENABLE_MASK | MODE_MASK | CLKSEL_MASK;

  //turn on the caches
  enableCache();

  //initialize all tasks's tids to -1
  //so we can identify if they have been
  //created later
  int i;
  for(i=0; i<MAXTASKS; i++) {
    taskArray[i].ID = -1;
  }

  *((int *)0x28) = (int)syscall_enter;
  *((int *)0x38) = (int)int_enter;
  nextTID = 0;
  kernMemStart = getSP();
  //leave 250KB of stack space for function calls
  kernMemStart -= 0xFA00;
  setIRQ_SP((int)kernMemStart);
  kernMemStart -= 0x10;		//give IRQ stack 8 words of space

  initQueue(readyQueue);
  freeMemStart = kernMemStart;

  //create the first user task
  active = NULL;
  Create_sys(7, firstTask);
  active = dequeue(readyQueue);
  active->state = ACTIVE;

  while(active != NULL) {
    bwprintf(COM2, "running task %d at address 0x%x, and stack at 0x%x\r", active->ID, active->returnAddress, active->SP);
    getNextRequest(active, activeRequest);
    handle(activeRequest);
  }

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
  newTD->SPSR = 0x50;
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
    case 0:					//IRQ
      *clockClear = 0;
      bwprintf(COM2, "100 ticks passed\r");
      makeTaskReady(active);
      active = getNextTask();
      break;
    case 1:					//Create
      if((int *)request->arg2 >= freeMemStart) {	//code pointer on the stack
        *(active->SP) = -3;
      }else if(request->arg1 >= NUMPRIO || request->arg1 < 0) {	//invalid priority
	*(active->SP) = -1;
      }else if(nextTID >= MAXTASKS) {				//out of TDs
        *(active->SP) = -2;
      }else{
        *(active->SP) = Create_sys(request->arg1, (void (*)())request->arg2);
      }
      makeTaskReady(active);
      active = getNextTask();
      break;
    case 2:				//MyTid
      *(active->SP) = active->ID;
      makeTaskReady(active);
      active = getNextTask();
      break;
    case 3:				//MyParentTid
      *(active->SP) = active->parentID;
      makeTaskReady(active);
      active = getNextTask();
      break;
    case 4:				//Pass
      makeTaskReady(active);
      active = getNextTask();
      break;
    case 5:				//Exit
      bwprintf(COM2, "task %d has exited\r", active->ID);
      //pop the send queue, returning -3
      while(active->sendQHead != NULL) {
        struct Task *queuedTask = active->sendQHead;
	active->sendQHead = queuedTask->next;
	queuedTask->next = NULL;
	*(queuedTask->SP) = -3;
	makeTaskReady(queuedTask);
      }
      active->state = ZOMBIE;
      active = getNextTask();
      break;
    case 6:				//Send
      if(request->arg1 >= MAXTASKS) {			//impossible TID
	*(active->SP) = -1;
	makeTaskReady(active);
      }else if(taskArray[request->arg1].ID == -1 || taskArray[request->arg1].state == ZOMBIE) {
	//task not created or has exited
	*(active->SP) = -2;
	makeTaskReady(active);
      }else if(request->arg1 == active->ID) {	//sending to yourself?
	*(active->SP) = -3;
	makeTaskReady(active);
      }else if(taskArray[request->arg1].state == SND_BL) {
        struct Task *receiverTask = &taskArray[request->arg1];
        memcpy(receiverTask->messageBuffer, (char *)request->arg2, 
		MIN(receiverTask->messageLength, request->arg3));
	*(receiverTask->senderTid) = active->ID;
	*(receiverTask->SP) = request->arg3;
	makeTaskReady(receiverTask);
        active->state = RPL_BL;
	active->replyBuffer = (char *)request->arg4;
	active->replyLength = request->arg5;
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
      }
      active = getNextTask();
      break;
    case 7:				//Receive
      if(active->sendQHead == NULL) {	//No tasks have sent
        active->state = SND_BL;
        active->messageBuffer = (char *)request->arg2;
        active->messageLength = request->arg3;
        active->senderTid = (int *)request->arg1;
      }else{
	struct Task *senderTask = active->sendQHead;
	active->sendQHead = senderTask->next;
	senderTask->next = NULL;
	memcpy((char *)request->arg2, senderTask->messageBuffer,
		MIN(senderTask->messageLength, request->arg3));
	*((int *)request->arg1) = senderTask->ID;
	*(active->SP) = senderTask->messageLength;
	senderTask->state = RPL_BL;
	makeTaskReady(active);
      }
      active = getNextTask();
      break;
    case 8:				//Reply
      if(request->arg1 >= MAXTASKS) {
	*(active->SP) = -1;
      }else if(taskArray[request->arg1].ID == -1 || taskArray[request->arg1].state == ZOMBIE) {
	*(active->SP) = -2;
      }else if(taskArray[request->arg1].state == RPL_BL) {
	struct Task *senderTask = &taskArray[request->arg1];
	memcpy(senderTask->replyBuffer, (char *)request->arg2,
		MIN(senderTask->replyLength, request->arg3));
	*(senderTask->SP) = request->arg3;
	*(active->SP) = 0;
	makeTaskReady(senderTask);
      }else{
	*(active->SP) = -3;
      }
      makeTaskReady(active);
      active = getNextTask();
      break;
  }
}

//scheduling functions
struct Task *getNextTask() {
  struct Task *nextTask = dequeue(readyQueue);
  nextTask->state = READY;
  return nextTask;
}
void makeTaskReady(struct Task *task) {
  task->state = READY;
  enqueue(readyQueue, task, task->priority);
}
