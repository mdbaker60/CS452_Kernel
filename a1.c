#include <a1.h>
#include <bwio.h>
#include <task.h>
#include <syscall.h>

struct Task *Create_sys(int priority, void (*code)());
//void schedule(struct Task *task, struct Request *request);
void schedule(struct Task *task);
void syscall_enter();
//void getNextRequest(struct Task *task, struct Request *request);
void getNextRequest(struct Task *task);

static int *freeMemStart;
static int *kernMemStart;
static struct Task *active;
static struct Request *activeRequest;

void sub() {
  bwprintf(COM2, "this is a test\r");
  Exit();
  bwprintf(COM2, "this is another test\r");
  Exit();
}

int main() {
  bwprintf(COM2, "main entered\r");
  *((int *)0x28) = (int)syscall_enter;
  bwprintf(COM2, "software interupt setup\r");

  kernMemStart = getSP();
  //leave 1KB of stack space for function calls
  kernMemStart -= 0x400;

  //Leave 1KB of space for kernel variables
  freeMemStart = kernMemStart - 0x400;
  bwprintf(COM2, "memory setup, starting at 0x%x\r", kernMemStart);

  //create a test TD
  active = Create_sys(0, sub);
  //kernMemStart -= sizeof(struct Request);
  //activeRequest = (struct Request *)kernMemStart;
  //activeRequest->ID = 0;

  bwprintf(COM2, "task created\r");
  getNextRequest(active);
  bwprintf(COM2, "back in the kernel\r");
  getNextRequest(active);
  bwprintf(COM2, "task complete\r");

  return 0;
}

void getNextRequest(struct Task *task) {
  schedule(task);
}

struct Task *Create_sys(int priority, void (*code)()) {
  kernMemStart -= sizeof(struct Task);
  struct Task *newTD = (struct Task *)kernMemStart;
  //initialize user task context
  newTD->SP = freeMemStart-56;		//loaded during user task schedule
  *(newTD->SP + 12) = (int)freeMemStart;	//stack pointer
  *(newTD->SP + 13) = (int)code;		//link register

  freeMemStart -= 0x400;	//give each task 1KB of stack space

  return newTD;
}
