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

/*void firstTask() {
  Create(6, NSInit);
  Create(6, InputInit);
  Create(6, OutputInit);
  Create(0, idleTask);

  char c;
  int i;
  for(i=0; i<5; i++) {
    //int c = Getc(2);
    c = bwgetc(COM2);
    //Putc(2, (char)c);
    bwprintf(COM2, "*");
  }

  Shutdown();
}*/

void server0() {
  RegisterAs("Server0");
  bwprintf(COM2, "Task %d: registered\r", MyTid());
  Destroy(MyTid());
}
void server1() {
  RegisterAs("Server1");
  bwprintf(COM2, "Task %d: registered\r", MyTid());
  Destroy(MyTid());
}
void server2() {
  RegisterAs("Server2");
  bwprintf(COM2, "Task %d: registered\r", MyTid());
  Destroy(MyTid());
}
void server3() {
  RegisterAs("Server3");
  bwprintf(COM2, "Task %d: registered\r", MyTid());
  Destroy(MyTid());
}
void server4() {
  RegisterAs("Server4");
  bwprintf(COM2, "Task %d: registered\r", MyTid());
  Destroy(MyTid());
}
void server5() {
  RegisterAs("Server5");
  bwprintf(COM2, "Task %d: registered\r", MyTid());
  Destroy(MyTid());
}
void server6() {
  RegisterAs("Server6");
  bwprintf(COM2, "Task %d: registered\r", MyTid());
  Destroy(MyTid());
}
void server7() {
  RegisterAs("Server7");
  bwprintf(COM2, "Task %d: registered\r", MyTid());
  Destroy(MyTid());
}
void server8() {
  RegisterAs("Server8");
  bwprintf(COM2, "Task %d: registered\r", MyTid());
  Destroy(MyTid());
}
void server9() {
  RegisterAs("Server9");
  bwprintf(COM2, "Task %d: registered\r", MyTid());
  Destroy(MyTid());
}

void firstTask() {
  Create(6, NSInit);
  Create(6, server0);
  Create(6, server1);
  Create(6, server2);
  Create(6, server3);
  Create(6, server4);
  Create(6, server5);
  Create(6, server6);
  Create(6, server7);
  Create(6, server8);
  Create(6, server9);
  Destroy(MyTid());
}
