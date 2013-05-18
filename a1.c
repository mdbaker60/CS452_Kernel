#include <a1.h>
#include <bwio.h>
#include <task.h>
#include <syscall.h>

struct Task *Create_sys(int priority, void (*code)());
void schedule(struct Task *task, struct Request *request);
void syscall_enter();
void getNextRequest(struct Task *task, struct Request *request);

static int *freeMemStart;
static int *kernMemStart;
static int nextTID;
static struct Task *active;
static struct Request *activeRequest;

void sub() {
  bwprintf(COM2, "this is a test\r");
  int myid = MyTid();
  bwprintf(COM2, "My ID is %d\r", myid);
  Exit();
}

void handle(struct Request *request);

int main() {
  *((int *)0x28) = (int)syscall_enter;
  nextTID = 0;

  kernMemStart = getSP();
  //leave 1KB of stack space for function calls
  kernMemStart -= 0x400;

  //Leave 1KB of space for kernel variables
  freeMemStart = kernMemStart - 0x400;

  //create a test TD
  active = Create_sys(0, sub);
  kernMemStart -= sizeof(struct Request);
  activeRequest = (struct Request *)kernMemStart;
  activeRequest->ID = 0;
  activeRequest->arg1 = 0;
  activeRequest->arg2 = 0;
  bwprintf(COM2, "task %d created\r", active->ID);

  getNextRequest(active, activeRequest);
  handle(activeRequest);
  getNextRequest(active, activeRequest);
  handle(activeRequest);

  return 0;
}

void getNextRequest(struct Task *task, struct Request *request) {
  schedule(task, request);
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
    case 0:
      bwprintf(COM2, "Exit called by task %d\r", active->ID);
      break;
    case 1:
      bwprintf(COM2, "MyTid called by task %d\r", active->ID);
      *(active->SP) = active->ID;
      break;
  }
}
