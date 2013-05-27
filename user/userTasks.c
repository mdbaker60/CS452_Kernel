#include <bwio.h>
#include <syscall.h>

/*void otherTask() {
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
}*/

void consumer() {
  int numFinished = 0, tid;
  char msg[100];
  while(numFinished < 4) {
    Receive(&tid, msg, 100);
    if(msg[0] == '!' && msg[1] == '\0') {
      numFinished++;
      bwprintf(COM2, "CONSUMER: producer %d has finished\r", tid-2);
      Reply(tid, "", 0);
    }else{
      bwprintf(COM2, "CONSUMER: producer %d sent message \"%s\"\r", tid-2, msg);
      switch(tid-2) {
	case 0:
	  Reply(tid, "Recived from 0", 15);
	  break;
	case 1:
	  Reply(tid, "Got message from 1", 19);
	  break;
	case 2:
	  Reply(tid, "This is a reply to 2", 21);
	  break;
	case 3:
	  Reply(tid, "I got a message from producer number three", 43);
	  break;
      }
    }
  }
  Exit();
}

void producer0() {
  int i,j;
  char reply[100];
  for(i=0; i<3; i++) {
    for(j=0; j<5; j++) {
      Pass();
    }
    Send(1, "From prod0", 11, reply, 100);
    bwprintf(COM2, "PRODUCER0: received reply \"%s\"\r", reply);
  }
  Send(1, "!", 2, reply, 100);
  Exit();
}

void producer1() {
  int i,j;
  char reply[100];
  for(i=0; i<2; i++) {
    for(j=0; j<2; j++) {
      Pass();
    }
    Send(1, "From prod1", 11, reply, 100);
    bwprintf(COM2, "PRODUCER1: received reply \"%s\"\r", reply);
  }
  Send(1, "!", 2, reply, 100);
  Exit();
}

void producer2() {
  int i,j;
  char reply[100];
  for(i=0; i<5; i++) {
    for(j=0; j<3; j++) {
      Pass();
    }
    Send(1, "From prod2", 11, reply, 100);
    bwprintf(COM2, "PRODUCER2: received reply \"%s\"\r", reply);
  }
  Send(1, "!", 2, reply, 100);
  Exit();
}

void producer3() {
  int i,j;
  char reply[100];
  for(i=0; i<4; i++) {
    for(j=0; j<6; j++) {
      Pass();
    }
    Send(1, "Message from prod3", 19, reply, 100);
    bwprintf(COM2, "PRODUCER3: received reply \"%s\"\r", reply);
  }
  Send(1, "!", 2, reply, 100);
  Exit();
}

void firstTask() {
  Create(0, consumer);
  Create(0, producer0);
  Create(0, producer1);
  Create(0, producer2);
  Create(0, producer3);
  Exit();
}
