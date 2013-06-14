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
  int *UART2Data = (int *)(UART2_BASE + UART_DATA_OFFSET);
  int i;

  while(true) {
    char c = (char)AwaitEvent(TERMIN_EVENT);
    AwaitEvent(TERMOUT_EVENT);
    *UART2Data = (int)c;
    if(c == 'q') Shutdown();
  }
}

void firstTask() {
  Create(6, NSInit);
  Create(6, CSInit);
  //Create(6, InputInit);
  Create(0, idleTask);
  Create(1, driver);

  Destroy(MyTid());
}
