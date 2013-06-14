#include <userTasks.h>
#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>
#include <clockServer.h>
#include <values.h>
#include <systemTasks.h>
#include <io.h>

void driver() {
  bwprintf(COM2, "driver is running - system about to shut down\r");
  Shutdown();
}

void firstTask() {
  Create(6, NSInit);
  Create(6, CSInit);
  Create(0, idleTask);
  Create(1, driver);

  Destroy(MyTid);
}
