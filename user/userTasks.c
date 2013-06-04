#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>
#include <clockServer.h>
#include <values.h>

void busyTask() {
  while(true) Pass();
}

void test() {
  bwprintf(COM2, "task created with id %d\r", MyTid());
  int i;
  for(i=0; i<7; i++) {
    bwprintf(COM2, "Waiting %ds\r", i);
    Delay(i*100);
    bwprintf(COM2, "Current time: %d\r", Time());
  }
  bwprintf(COM2, "Waiting until time 2345\r");
  DelayUntil(2345);
  bwprintf(COM2, "Done\r");
  Exit();
}

void firstTask() {
  Create(7, NSInit);
  Create(7, CSInit);
  Create(1, test);
  Create(0, busyTask);
  Exit();
}
