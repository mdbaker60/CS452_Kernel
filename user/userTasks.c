#include <bwio.h>
#include <syscall.h>

void otherTask() {
  int myid = MyTid();
  int parentid = MyParentTid();
  bwprintf(COM2, "My TID is %d, and my parent's is %d\r", myid, parentid);
  Pass();
  bwprintf(COM2, "My TID is %d, and my parent's is %d\r", myid, parentid);
  Exit();
}

void firstTask() {
  int childid = Create(0, otherTask);
  bwprintf(COM2, "Created: %d.\r", childid);
  childid = Create(0, otherTask);
  bwprintf(COM2, "Created: %d.\r", childid);
  childid = Create(2, otherTask);
  bwprintf(COM2, "Created: %d.\r", childid);
  childid = Create(2, otherTask);
  bwprintf(COM2, "Created: %d.\r", childid);
  bwprintf(COM2, "First: exiting\r");
  Exit();
}
