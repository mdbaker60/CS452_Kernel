#include <kern.h>
#include <bwio.h>
#include <task.h>
#include <syscall.h>
#include <userTasks.h>
#include <queue.h>
#include <memcpy.h>
#include <ts7200.h>
#include <values.h>

#ifdef DEBUG
#define DEBUGPRINT(...) bwprintf(COM2, __VA_ARGS__);
#else
#define DEBUGPRINT(...)	
#endif

static int *userStacks;
static int *kernMemStart;
static struct Task *active;
static struct PriorityQueue *readyQueue;
static struct Request *activeRequest;
static struct Task *taskArray;
static struct Task **waitingTasks;
static struct Task *firstFree;
static struct Task *lastFree;
static int totalTime;
static int *eventStatus;

static char *kernOut;
static int kOutHead;
static int kOutTail;

int main() {
  struct Request kactiveRequest;
  struct PriorityQueue kreadyQueue;
  struct Task ktaskArray[MAXTASKS];
  struct Task *kwaitingTasks[NUMEVENTS];
  int keventStatus[NUMEVENTS];
  char kOut[1000];
  activeRequest = &kactiveRequest; 
  readyQueue = &kreadyQueue;
  taskArray = ktaskArray;
  waitingTasks = kwaitingTasks;
  eventStatus = keventStatus;
  kernOut = kOut;
  kOutHead = kOutTail = 0;

  //initialize clock
  int *clockControl = (int *)(TIMER3_BASE + CRTL_OFFSET);
  int *clockLoad = (int *)(TIMER3_BASE + LDR_OFFSET);
  *clockLoad = 5079;
  *clockControl = MODE_MASK | CLKSEL_MASK | ENABLE_MASK;
  //initialize the UARTs
  int *UART2Control = (int *)(UART2_BASE + UART_CTLR_OFFSET);
  *UART2Control |= RIEN_MASK;
  //turn off the FIFOs
  int *UART2FIFOControl = (int *)(UART2_BASE + UART_LCRH_OFFSET);
  *UART2FIFOControl &= ~(FEN_MASK);
  //turn on interrupts from the clock and UARTs
  int *ICU2Control = (int *)(ICU2_BASE + ENBL_OFFSET);
  *ICU2Control |= (CLK3_MASK | UART2_MASK);
  //enable the 40-bit clock
  clockControl = (int *)(TIMER4_HIGH);
  *clockControl &= TIMER4_ENABLE_MASK;
  int *counterValue = (int *)(TIMER4_LOW);
  totalTime = 0;

  //turn on the caches
  enableCache();

  //initialize all tasks's tids to -1
  //so we can identify if they have been
  //created later
  int i;
  for(i=0; i<MAXTASKS-1; i++) {
    taskArray[i].ID = i - GEN_UNIT;
    taskArray[i].next = &taskArray[i+1];
  }
  firstFree = &taskArray[0];
  lastFree = &taskArray[MAXTASKS-1];
  lastFree->next = NULL;
  lastFree->ID = MAXTASKS - 1 - GEN_UNIT;
  //initialize all wating tasks to NULL
  for(i=0; i<NUMEVENTS; i++) {
    waitingTasks[i] = NULL;
    eventStatus[i] = -1;
  }

  *((int *)0x28) = (int)syscall_enter;
  *((int *)0x38) = (int)int_enter;
  *((int *)0x3C) = (int)int_enter;
  kernMemStart = getSP();
  //leave 250KB of stack space for function calls
  kernMemStart -= 0xFA00;
  setIRQ_SP((int)kernMemStart);
  kernMemStart -= 0x8;		//give IRQ stack 8 words of space

  initQueue(readyQueue);
  userStacks = kernMemStart - MAXTASKS*(0xFA00);
  DEBUGPRINT("user stacks start at 0x%x\r", userStacks);

  //create the first user task
  active = NULL;
  Create_sys(6, firstTask);
  active = dequeue(readyQueue);
  active->state = ACTIVE;

  int startTime, endTime;
  while(active != NULL) {
    startTime = *counterValue;
    getNextRequest(active, activeRequest);
    endTime = *counterValue;
    active->totalTime += endTime - startTime;
    totalTime += endTime - startTime;
    handle(activeRequest);
  }

  for(i=0; i<MAXTASKS; i++) {
    if(taskArray[i].state != ZOMBIE && taskArray[i].ID >= 0) {
      DEBUGPRINT("WARNING: task %d has not exited(in state %d)\r", taskArray[i].ID, taskArray[i].state);
    }
  }

  //turn off interupts and clocks
  //*UART2Control &= ~(RIEN_MASK | TIEN_MASK);
  //int *ICU1Clear = (int *)(ICU1_BASE + ENCL_OFFSET);
  int *ICU2Clear = (int *)(ICU2_BASE + ENCL_OFFSET);
  //*ICU2Control = 0x0;
  //i*ICU1Clear = ICU1_ALL_MASK;
  //*ICU2Clear = ICU2_ALL_MASK;
  *ICU2Clear = (CLK3_MASK | UART2_MASK);
  *clockControl &= ~(TIMER4_ENABLE_MASK);
  clockControl = (int *)(TIMER3_BASE + CRTL_OFFSET);
  *clockControl &= ~(ENABLE_MASK);

  //turn back on the FIFOs
  *UART2FIFOControl |= FEN_MASK;

  return 0;
}

int Create_sys(int priority, void (*code)()) {
  if(firstFree == NULL) return -2;
  struct Task *newTD = firstFree;
  firstFree = firstFree->next;
  newTD->ID += GEN_UNIT;
  int myID = newTD->ID & INDEX_MASK;
  int *myStack = userStacks + (myID)*(0xFA00);
  //initialize user task context
  newTD->SP = myStack-56;		//loaded during user task schedule
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
  newTD->last = NULL;
  newTD->sendQHead = NULL;
  newTD->totalTime = 0;
  newTD->event = -1;
  *(newTD->SP + 12) = (int)myStack;	//stack pointer
  *(newTD->SP + 13) = (int)Exit;
  enqueue(readyQueue, newTD, priority);

  return newTD->ID;
}

void handle(struct Request *request) {
  int i;
  switch(request->ID) {
    case INTERRUPT:
      handleInterrupt();
      makeTaskReady(active);
      active = getNextTask();
      break;
    case CREATE:
      if((int *)request->arg2 >= &userStacks[MAXTASKS*(0xFA00)]) {	//code pointer on the stack
        *(active->SP) = -3;
      }else if(request->arg1 >= NUMPRIO || request->arg1 < 0) {	//invalid priority
	*(active->SP) = -1;
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
      //print out percent time used by this task
      DEBUGPRINT("Task %d: used %d percent of CPU time\r", active->ID, (100*(active->totalTime))/totalTime);
      active->state = ZOMBIE;
      active = getNextTask();
      break;
    case SEND:
      if((request->arg1 & INDEX_MASK) >= MAXTASKS) {			//impossible TID
	*(active->SP) = -1;
	makeTaskReady(active);
      }else if(taskArray[request->arg1 & INDEX_MASK].ID != request->arg1 || 
		taskArray[request->arg1 & INDEX_MASK].state == ZOMBIE) {
	//task not created or has exited
	*(active->SP) = -2;
	makeTaskReady(active);
      }else if(request->arg1 == active->ID) {	//sending to yourself?
	*(active->SP) = -3;
	makeTaskReady(active);
      }else if(taskArray[request->arg1 & INDEX_MASK].state == SND_BL) {
        struct Task *receiverTask = &taskArray[request->arg1 & INDEX_MASK];
        memcpy(receiverTask->messageBuffer, (char *)request->arg2, 
		MIN(receiverTask->messageLength, request->arg3));
	*(receiverTask->senderTid) = active->ID;
	*(receiverTask->SP) = request->arg3;
	makeTaskReady(receiverTask);
        active->state = RPL_BL;
	active->replyBuffer = (char *)request->arg4;
	active->replyLength = request->arg5;
      }else{
	struct Task *receiverTask = &taskArray[request->arg1 & INDEX_MASK];
	active->state = RCV_BL;
	active->receiverTid = request->arg1;	//used if Destroy is called while RCV_BL
	active->messageBuffer = (char *)request->arg2;
	active->messageLength = request->arg3;
	active->replyBuffer = (char *)request->arg4;
	active->replyLength = request->arg5;
	if(receiverTask->sendQHead == NULL) {
	  receiverTask->sendQHead = receiverTask->sendQTail = active;
	}else{
	  (receiverTask->sendQTail)->next = active;
	  active->last = receiverTask->sendQTail;
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
	(active->sendQHead)->last = NULL;
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
      if((request->arg1 & INDEX_MASK) >= MAXTASKS) {
	*(active->SP) = -1;
      }else if(taskArray[request->arg1 & INDEX_MASK].ID != request->arg1 || 
		taskArray[request->arg1].state == ZOMBIE) {
	*(active->SP) = -2;
      }else if(taskArray[request->arg1 & INDEX_MASK].state == RPL_BL) {
	struct Task *senderTask = &taskArray[request->arg1 & INDEX_MASK];
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
      if(eventStatus[request->arg1] != -1) {
	*(active->SP) = eventStatus[request->arg1];
	eventStatus[request->arg1] = -1;
	makeTaskReady(active);
      }else if(waitingTasks[request->arg1] == NULL) {
	waitingTasks[request->arg1] = active;
	active->state = EVT_BL;
	active->event = request->arg1;
	if(request->arg1 == TERMOUT_EVENT) {
	  int *UART2Control = (int *)(UART2_BASE + UART_CTLR_OFFSET);
	  *UART2Control |= TIEN_MASK;
	}
      }else{
        *(active->SP) = -4;
	makeTaskReady(active);
      }
      active = getNextTask();
      break;
    case DESTROY:
      destroyTask(request->arg1);
      if(active->ID != request->arg1) {
	makeTaskReady(active);
      }
      active = getNextTask();
      break;
    case SHUTDOWN:
      /*for(i=0; i<MAXTASKS; i++) {
	struct Task *curTask = &taskArray[i];
	if(curTask->state != ZOMBIE && curTask->ID >= 0) {
	  destroyTask(curTask->ID);
	}
      }*/
      //for(i=0; i<NUMEVENTS; i++) {
	//waitingTasks[i] = NULL;
      //}
      for(i=0; i<MAXTASKS; i++) {
	if(taskArray[i].state != ZOMBIE && taskArray[i].ID >= 0) {
  	  destroyTask(taskArray[i].ID);
	}
      }
      active = NULL;
      break;
  }
}

//scheduling functions
struct Task *getNextTask() {
  struct Task *nextTask = dequeue(readyQueue);
  nextTask->state = ACTIVE;
  return nextTask;
}
void makeTaskReady(struct Task *task) {
  task->state = READY;
  enqueue(readyQueue, task, task->priority);
}

int destroyTask(int Tid) {
  //pop the send queue, returning -3
  struct Task *toDestroy = &taskArray[Tid & INDEX_MASK];
  if(toDestroy->state == READY) {
    removeFromQueue(readyQueue, toDestroy);
  }
  while(toDestroy->sendQHead != NULL) {
    struct Task *queuedTask = toDestroy->sendQHead;
    toDestroy->sendQHead = queuedTask->next;
    queuedTask->next = NULL;
    *(queuedTask->SP) = -3;
    makeTaskReady(queuedTask);
  }
  //if task is event blocked, remove it from event array
  if(toDestroy->state == EVT_BL) {
    waitingTasks[toDestroy->event] = NULL;
  }
  //if task is receive blocked, remove it from the receving taks's send queue
  if(toDestroy->state == RCV_BL) {
    if(toDestroy->next == NULL && toDestroy->last == NULL) {
      struct Task *receiver = &taskArray[toDestroy->receiverTid & INDEX_MASK];
      receiver->sendQHead = receiver->sendQTail = NULL;
    }else if(toDestroy->next == NULL) {
      (toDestroy->last)->next = NULL;
      struct Task *receiver = &taskArray[toDestroy->receiverTid & INDEX_MASK];
      receiver->sendQTail = toDestroy->last;
    }else if(toDestroy->last == NULL) {
      (toDestroy->next)->last = NULL;
      struct Task *receiver = &taskArray[toDestroy->receiverTid & INDEX_MASK];
      receiver->sendQHead = toDestroy->next;
    }else{
      (toDestroy->next)->last = toDestroy->last;
      (toDestroy->last)->next = toDestroy->next;
    }
  }
  //print out percent time used by this task
  DEBUGPRINT("Task %d: used %d percent of CPU time*\r", toDestroy->ID, (100*(toDestroy->totalTime))/totalTime);
  toDestroy->state = ZOMBIE;
  if(firstFree == NULL) {
    firstFree = lastFree = toDestroy;
  }else{
    lastFree->next = toDestroy;
    lastFree = toDestroy;
  }

  return 0;
}

void handleInterrupt() {
  int *ICU2Status = (int *)(ICU2_BASE + STAT_OFFSET);
  if(*ICU2Status & UART2_MASK) {
    int *UART2Status = (int *)(UART2_BASE + UART_INTR_OFFSET);
    if(*UART2Status & RIS_MASK) {
      int *UART2Data = (int *)(UART2_BASE + UART_DATA_OFFSET);
      if(waitingTasks[TERMIN_EVENT] != NULL) {
        *(waitingTasks[TERMIN_EVENT]->SP) = *UART2Data;
        makeTaskReady(waitingTasks[TERMIN_EVENT]);
        waitingTasks[TERMIN_EVENT] = NULL;
      }else{
        eventStatus[TERMIN_EVENT] = *UART2Data;
      }
    }else if(*UART2Status & TIS_MASK) {
      if(kOutHead != kOutTail) {
	kOutputChar();
	return;
      }
      int *UART2Control = (int *)(UART2_BASE + UART_CTLR_OFFSET);
      *UART2Control &= ~TIEN_MASK;
      if(waitingTasks[TERMOUT_EVENT] != NULL) {
	*(waitingTasks[TERMOUT_EVENT]->SP) = 0;
	makeTaskReady(waitingTasks[TERMOUT_EVENT]);
	waitingTasks[TERMOUT_EVENT] = NULL;
      }else{
	eventStatus[TERMOUT_EVENT] = 0;
      }
    }
  }else if(*ICU2Status & UART1_MASK){
    int *UART1_FLAG = (int *)UART1_BASE + UART_FLAG_OFFSET;
    if (*UART1_FLAG & CTS_MASK){
      if(waitingTasks[TRAIOUT_EVENT] != NULL) {
       makeTaskReady(waitingTasks[TRAIOUT_EVENT]);
       waitingTasks[TRAIOUT_EVENT] = NULL; 
      }
    }
    
  }else if(*ICU2Status & CLK3_MASK) {
    int *clockClear = (int *)(TIMER3_BASE + CLR_OFFSET);
    *clockClear = 0;
    if(waitingTasks[CLOCK_EVENT] != NULL) {
      *(waitingTasks[CLOCK_EVENT]->SP) = 0;
      makeTaskReady(waitingTasks[CLOCK_EVENT]);
      waitingTasks[CLOCK_EVENT] = NULL;
    }else{
      eventStatus[CLOCK_EVENT] = 0;
    }
  }
}


void kprint(char *str) {
  while(*str != '\0') {
    kernOut[kOutHead++] = *str;
    kOutHead %= 1000;
    ++str;
  }

  //turn on interrupts
  int *UART2Control = (int *)(UART2_BASE + UART_CTLR_OFFSET);
  *UART2Control |= TIEN_MASK;
}

void kOutputChar() {
  int *UART2Data = (int *)(UART2_BASE + UART_DATA_OFFSET);
  *UART2Data = kernOut[kOutTail++];
  kOutTail %= 1000;

  //possible disable interrupts
  if(kOutHead == kOutTail && waitingTasks[TERMOUT_EVENT] == NULL) {
    int *UART2Control = (int *)(UART2_BASE + UART_CTLR_OFFSET);
    *UART2Control &= ~TIEN_MASK;
  }
}
