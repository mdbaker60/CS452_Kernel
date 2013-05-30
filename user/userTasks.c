#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>
void producer() {
  char reply[100];
  char test[10];
  int i;
  for(i=0; i<9; i++) {
    test[i] = '1';
  }
  for(i=0; i<100; i++) {
    reply[i] = '*';
  }
  test[9] = '\0';
  Send(2, "This message is waaaaay too long!", 34, reply, 10);
  reply[9] = '\0';
  bwprintf(COM2, "received reply \"%s\" and test is \"%s\"\r", reply, test);
  for(i=0; i<100; i++) {
    bwputc(COM2, reply[i]);
  }
  Exit();
}

void consumer() {
  char msg[100];
  int id;
  char test[10];
  int i;
  for(i=0; i<9; i++) {
    test[i] = '2';
  }
  for(i=0; i<100; i++) {
    msg[i] = '*';
  }
  test[9] = '\0';
  Receive(&id, msg, 10);
  msg[9] = '\0';
  bwprintf(COM2, "received message \"%s\" from task %d\r", msg, id);
  Reply(id, "This is a very long reply", 26);
  bwprintf(COM2, "send reply, and test is \"%s\"\r", test);
  for(i=0; i<100; i++) {
    bwputc(COM2, msg[i]);
  }
  Exit();
}
void regger(){
	//Registers with the nameserver
	char* who = "RPS Server";
	bwprintf(COM2, "Starting server\r");
	RegisterAs(who);
	bwprintf(COM2, "PASSING:\r");		
	Pass();
}
void reqqer(){
	char* who = "RPS Server";
	char ret[100];
	bwprintf(COM2, "Starting client\r");
	whoIs(who);
	if(ret == '\0') bwprintf(COM2, "nothing found\r");
}
void firstTask() {
 // Create(0, producer);
  //Create(0, consumer);
  Create(2, NSInit);
  Create(5, regger);
  Create(5, reqqer);
  Pass();
  Exit();
}
