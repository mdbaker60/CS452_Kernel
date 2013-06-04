#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>

void busyTask() {
  while(1) Pass();
}

void test() {
  bwprintf(COM2, "task created with id %d\r", MyTid());
  int i,j;
  for(i=0; i<10; i++) {
    for(j=0; j<100; j++) {
      AwaitEvent(0);
    }
    bwprintf(COM2, "%d seconds have passed\r", i+1);
  }
  Exit();
}

void firstTask() {
  //Create(7,NSInit);
  Create(0, test);
  Create(0, busyTask);
  Exit();
}
