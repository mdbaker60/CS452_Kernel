#include <a1.h>
#include <bwio.h>
#include <task.h>
#include <syscall.h>

static int *freeMemStart;
static int *kernMemStart;
static int nextTID = 3;
static struct Task *active;
static struct Request *activeRequest;
static struct Queue *readyQueue;

void sub() {
  int myid = MyTid();
  bwprintf(COM2, "My ID is %d\r", myid);
  Exit();
}

void handle(struct Request *request);

int main() {
  *((int *)0x28) = (int)syscall_enter;

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
  active = Create_sys(0, sub);
  enqueue((int *)Create_sys(0, sub), readyQueue);
  while(active != NULL) {
    getNextRequest(active, activeRequest);
    handle(activeRequest);
  }
  //struct Task *second = Create_sys(0, sub);

  /*getNextRequest(active, activeRequest);
  handle(activeRequest);
  struct Task *temp = active;
  active = second;
  second = temp;
  getNextRequest(active, activeRequest);
  handle(activeRequest);
  temp = active;
  active = second;
  second = temp;
  getNextRequest(active, activeRequest);
  handle(activeRequest);
  temp = active;
  active = second;
  second = temp;
  getNextRequest(active, activeRequest);
  handle(activeRequest);*/

  return 0;
}

struct Task *Create_sys(int priority, void (*code)()) {
  kernMemStart -= sizeof(struct Task);
  struct Task *newTD = (struct Task *)kernMemStart;
  //initialize user task context
  newTD->SP = freeMemStart-56;		//loaded during user task schedule
  newTD->ID = nextTID++;
  *(newTD->SP + 12) = (int)freeMemStart;	//stack pointer
  *(newTD->SP + 13) = (int)code;		//link register

  freeMemStart -= 0x400;	//give each task 1KB of stack space

  return newTD;
}

void handle(struct Request *request) {
  switch(request->ID) {
    case 0:				//Exit
      active = (struct Task *)dequeue(readyQueue);
      break;
    case 1:				//MyTid
      *(active->SP) = active->ID;
      enqueue((int *)active, readyQueue);
      active = (struct Task *)dequeue(readyQueue);
      break;
  }
}
