#include <userTasks.h>
#include <systemTasks.h>
#include <io.h>
#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>
#include <clockServer.h>
#include <values.h>
#include <ts7200.h>

void firstTask() {
  Create(6, NSInit);
  Create(6, InputInit);
  Create(6, OutputInit);
  Create(0, idleTask);
  char c = '0';
  int i;
  for(i=0; i<5; i++) {
    //int c = Getc(2);
    c = bwgetc(COM2);
    //Putc(2, (char)c);
    bwprintf(COM2, "*");
  }

  Shutdown();
}
