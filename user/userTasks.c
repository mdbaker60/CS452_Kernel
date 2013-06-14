#include <userTasks.h>
#include <systemTasks.h>
#include <io.h>
#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>
#include <clockServer.h>
#include <values.h>
#include <ts7200.h>
#include <string.h>

void driver() {
  Shutdown();
}

void firstTask() {
  Create(6, NSInit);
  Create(6, InputInit);
  Create(6, OutputInit);
  Create(0, idleTask);
  Create(1, driver);

  Destroy(MyTid());
}
