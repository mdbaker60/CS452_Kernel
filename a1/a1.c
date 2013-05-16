#include <a1.h>
#include <bwio.h>
#include <task.h>
#include <syscall.h>

struct Task *Create_sys(int priority, void (*code)());
struct Task *schedule(struct Task *task);
void syscall_enter();

static int *freeMemStart;
static int *kernMemStart;
static struct Task *curTask;

void sub() {
  bwprintf(COM2, "this is a test\r");
  Exit();
}

int main() {
  bwprintf(COM2, "main entered\r");
  *((int *)0x28) = (int)syscall_enter;
  bwprintf(COM2, "software interupt setup\r");

  kernMemStart = getSP();
  bwprintf(COM2, "stack pointer retrieved\r");
  //Leave 1KB of free memory for kernel stack
  freeMemStart = kernMemStart - 0x400;
  bwprintf(COM2, "memory setup\r");

  //create a test TD
  struct Task *newTD = Create_sys(0, sub);
  bwprintf(COM2, "task created\r");
  newTD = schedule(newTD);
  bwprintf(COM2, "task completed\r");

  return 0;
}

struct Task *Create_sys(int priority, void (*code)()) {
  struct Task *newTD = (struct Task *)freeMemStart;
  freeMemStart -= sizeof(struct Task);
  newTD->SP = (int)freeMemStart;
  newTD->LR = (int)code;
  freeMemStart -= 0x400;	//give each task 1KB of stack space

  return newTD;
}
