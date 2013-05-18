#include <a1.h>
#include <bwio.h>
#include <task.h>
#include <syscall.h>

struct Task *Create_sys(int priority, void (*code)());
struct Task *schedule(struct Task *task);
void syscall_enter();
void getNextRequest(struct Task *task);

static int *freeMemStart;
static int *kernMemStart;
static struct Task *active;

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
  kernMemStart++;


  bwprintf(COM2, "stack pointer retrieved\r");
  //Leave 1KB of free memory for kernel stack
  freeMemStart = kernMemStart - 0x400;
  bwprintf(COM2, "memory setup\r");

  //create a test TD
  struct Task *active = Create_sys(0, sub);
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
  struct Task *newTD = (struct Task *)kernMemStart;
  kernMemStart -= sizeof(struct Task);
  //initialize user task context
  newTD->SP = freeMemStart-40;		//loaded during user task schedule
  *(newTD->SP + 12) = freeMemStart;	//stack pointer
  *(newTD->SP + 13) = code;		//link register

  freeMemStart -= 0x400;	//give each task 1KB of stack space

  return newTD;
}
