#include <kern.h>
#include <bwio.h>
#include <task.h>
#include <syscall.h>
#include <userTasks.h>
#include <queue.h>
#include <memcpy.h>
#include <ts7200.h>

static int *freeMemStart;
static int *kernMemStart;
static int nextTID;
static struct Task *active;
static struct PriorityQueue *readyQueue;
static struct Request *activeRequest;
static struct Task *taskArray;
static struct Task **waitingTasks;

int main() {
  struct Request kactiveRequest;
  struct PriorityQueue kreadyQueue;
  struct Task ktaskArray[MAXTASKS];
  struct Task *kwaitingTasks[NUMEVENTS];
  activeRequest = &kactiveRequest; 
  readyQueue = &kreadyQueue;
  taskArray = ktaskArray;
  waitingTasks = kwaitingTasks;

  //initialize clock
  int *clockControl = (int *)(TIMER3_BASE + CRTL_OFFSET);
  int *clockLoad = (int *)(TIMER3_BASE + LDR_OFFSET);
  *clockLoad = 5079;
  *clockControl = ENABLE_MASK | MODE_MASK | CLKSEL_MASK;
  //turn on clock interrupts
  int *intControl = (int *)(ICU2_BASE + ENBL_OFFSET);
  *intControl = CLK3_MASK;

  //turn on the caches
  enableCache();

  //initialize all tasks's tids to -1
  //so we can identify if they have been
  //created later
  int i;
  for(i=0; i<MAXTASKS; i++) {
    taskArray[i].ID = -1;
  }
  //initialize all wating tasks to NULL
  for(i=0; i<NUMEVENTS; i++) {
    waitingTasks[i] = NULL;
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
    case INTERRUPT:
      handleInterrupt();
      makeTaskReady(active);
      active = getNextTask();
      break;
    case CREATE:
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
    case MYTID:
      *(active->SP) = active->ID;
      makeTaskReady(active);
      active = getNextTask();
      break;
    case MYPARENTTID:
      *(active->SP) = active->parentID;
      makeTaskReady(active);
      active = getNextTask();
      break;
    case PASS:
      makeTaskReady(active);
      active = getNextTask();
      break;
    case EXIT:
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
    case SEND:
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
    case RECEIVE:
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
    case REPLY:
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
    case AWAITEVENT:
      if(waitingTasks[request->arg1] == NULL) {
	waitingTasks[request->arg1] = active;
	active->state = EVT_BL;
      }else{
        *(active->SP) = -4;
	makeTaskReady(active);
      }
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

void handleInterrupt() {
  int *status = (int *)(ICU2_BASE + STAT_OFFSET);
  if((int)status | CLK3_MASK) {
    int *clockClear = (int *)(TIMER3_BASE + CLR_OFFSET);
    *clockClear = 0;
    if(waitingTasks[CLOCK_EVENT] != NULL) {
      *(waitingTasks[CLOCK_EVENT]->SP) = 0;
      makeTaskReady(waitingTasks[CLOCK_EVENT]);
      waitingTasks[CLOCK_EVENT] = NULL;
    }
  }
}
