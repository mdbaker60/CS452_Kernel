#include <userTasks.h>
#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>
#include <clockServer.h>
#include <values.h>
#include <systemTasks.h>
#include <io.h>

void firstTask() {
  Create(6, NSInit);
  Create(6, CSInit);
  Create(6, InputInit);
  Create(6, OutputInit);
  Create(0, idleTask);

  int i;
  for(i=0; i<5; i++) {
    bwgetc(COM2);
    bwputc(COM2, '*');
  }

  Shutdown();
}
