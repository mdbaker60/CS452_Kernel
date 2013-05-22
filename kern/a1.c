#include <a1.h>
#include <bwio.h>
#include <task.h>
#include <syscall.h>
#include <userTasks.h>

static int *freeMemStart;
static int *kernMemStart;
static int nextTID;
static struct Task *active;
static struct Request *activeRequest;
static struct Queue *readyQueue;

int main() {
  *((int *)0x28) = (int)syscall_enter;
  nextTID = 0;

  kernMemStart = getSP();
  //leave 1KB of stack space for function calls
  kernMemStart -= 0x400;
  //Leave 1KB of space for kernel variables
  freeMemStart = kernMemStart - 0x400;

  //initialize the readyQueue
  kernMemStart -= sizeof(struct Queue);
  readyQueue = (struct Queue *)kernMemStart;
  init(readyQueue);

  //initialize the active request structure
  kernMemStart -= sizeof(struct Request);
  activeRequest = (struct Request *)kernMemStart;

  //create a test TD
  active = NULL;
  active = Create_sys(1, firstTask);
  while(active != NULL) {
    getNextRequest(active, activeRequest);
    handle(activeRequest);
  }

  return 0;
}

struct Task *Create_sys(int priority, void (*code)()) {
  kernMemStart -= sizeof(struct Task);
  struct Task *newTD = (struct Task *)kernMemStart;
  //initialize user task context
  newTD->SP = freeMemStart-56;		//loaded during user task schedule
  newTD->ID = nextTID++;
  newTD->SPSR = 0xD0;
  newTD->parent = active;
  newTD->returnAddress = (int)code;
  *(newTD->SP + 12) = (int)freeMemStart;	//stack pointer

  freeMemStart -= 0x400;	//give each task 1KB of stack space

  return newTD;
}

void handle(struct Request *request) {
  //used for Create
  struct Task *newTask;

  switch(request->ID) {
    case 0:				//Create
      newTask = Create_sys(request->arg1, (void (*)())request->arg2);
      *(active->SP) = newTask->ID;
      enqueue((int *)newTask, readyQueue);
      enqueue((int *)active, readyQueue);
      active = (struct Task *)dequeue(readyQueue);
      break;
    case 1:				//MyTid
      *(active->SP) = active->ID;
      enqueue((int *)active, readyQueue);
      active = (struct Task *)dequeue(readyQueue);
      break;
    case 2:				//MyParentTid
      if(active->parent == NULL) {
	*(active->SP) = -1;
      }else{
	*(active->SP) = (active->parent)->ID;
      }
      enqueue((int *)active, readyQueue);
      active = (struct Task *)dequeue(readyQueue);
      break;
    case 3:				//Pass
      enqueue((int *)active, readyQueue);
      active = (struct Task *)dequeue(readyQueue);
      break;
    case 4:				//Exit
      active = (struct Task *)dequeue(readyQueue);
      break;
  }
}
