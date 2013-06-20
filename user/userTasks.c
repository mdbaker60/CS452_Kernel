#include <userTasks.h>
#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>
#include <clockServer.h>
#include <values.h>
#include <systemTasks.h>
#include <io.h>
#include <ts7200.h>

void driver() {
  int i;

  printf("This is a test number: %d\r", 23456);

  while(true) {
    int c = Getc(2);
    Putc(2, c);
    if(c == 'q') break;
  }
  Putc(2, '$');

  Shutdown();
}

void firstTask() {
  Create(6, NSInit);
  Create(6, CSInit);
  Create(6, InputInit);
  Create(6, OutputInit);
  Create(0, idleTask);
  Create(1, driver);

  Destroy(MyTid());
}
