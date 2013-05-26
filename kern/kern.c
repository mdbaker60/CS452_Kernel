#include <kern.h>
#include <bwio.h>
#include <task.h>
#include <syscall.h>
#include <userTasks.h>
#include <queue.h>

static int *freeMemStart;
static int *kernMemStart;
static struct Task *active;
static struct PriorityQueue *readyQueue;
static struct Task *taskArray;

int main() {
  struct Request activeRequest;
  struct PriorityQueue kreadyQueue;
  struct Task ktaskArray[MAXTASKS];
  readyQueue = &kreadyQueue;
  taskArray = ktaskArray;

  *((int *)0x28) = (int)syscall_enter;
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

  return 0;
}

int Create_sys(int priority, void (*code)()) {
  static int nextTID = 0;

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
  }
}
