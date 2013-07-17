#include <userTasks.h>
#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>
#include <clockServer.h>
#include <values.h>
#include <systemTasks.h>
#include <io.h>
#include <ts7200.h>
#include <term.h>
#include <trackServer.h>

void firstTask() {
  Create(6, NSInit);
  Create(0, idleTask);
  Create(6, CSInit);
  Create(6, InputInit);
  Create(6, OutputInit);
  Create(6, TrackServerInit);
  Create(6, DSInit);
  Create(1, terminalDriver);

  Destroy(MyTid());
}
