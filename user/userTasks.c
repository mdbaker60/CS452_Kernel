#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>

void busyTask() {
  while(1) {};
}

void test() {
  bwprintf(COM2, "task created with id %d\r", MyTid());
  Exit();
}

void firstTask() {
  //Create(7,NSInit);
  Create(0, test);
  Create(0, busyTask);
  Exit();
}
