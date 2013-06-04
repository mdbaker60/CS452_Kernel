#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>
#include <clockServer.h>

void busyTask() {
  while(1) Pass();
}

void test() {
  bwprintf(COM2, "task created with id %d\r", MyTid());
  int i;
  for(i=0; i<7; i++) {
    bwprintf(COM2, "Waiting %ds\r", i);
    Delay(i*100);
    bwprintf(COM2, "Current time: %d\r", Time());
  }
  bwprintf(COM2, "Waiting until time 2345\r");
  DelayUntil(2345);
  bwprintf(COM2, "Done\r");
  Exit();
}

struct msg{
  int delayTime;
  int delayNum;
};
void clientTask(){
	int parent = MyParentTid();
	int tid = MyTid();
	struct msg params;
	int i = 1;
	//Priority
	//Delay time
	//Number of delays
	//Send a message to the first task which returns CS parameters
	Send(parent, "0", sizeof(2), (char *) &params, sizeof(struct msg));
	bwprintf(COM2, "%d: Delay Time %d with %d delays\r", tid, params.delayTime, params.delayNum);
	while(params.delayNum >= i){
		Delay(params.delayTime);
		bwprintf(COM2, "%d: Delay Time: %d, Finished delay: %d\r", tid, params.delayTime, i);
		i++; 
	}	
	Exit();

}
void firstTask() {
  int src=-1;
  struct msg buf;	
  char * in [3]; //unused
  Create(7, NSInit);
  Create(7, CSInit);
  Create(3, clientTask);
  Create(4, clientTask);
  Create(5, clientTask);
  Create(6, clientTask);
  Create(0, busyTask);
  buf.delayTime = 10;
  buf.delayNum = 20; 
  Receive(&src, (char *) in, sizeof(struct msg));
  Reply(src, (char *)&buf, sizeof(struct msg)); 
  buf.delayTime = 23;
  buf.delayNum = 9;
  Receive(&src, (char *) in, sizeof(struct msg));
  Reply(src, (char *)&buf, sizeof(struct msg)); 
  buf.delayTime = 33;
  buf.delayNum = 6;
  Receive(&src, (char *) in, sizeof(struct msg));
  Reply(src, (char *)&buf, sizeof(struct msg)); 
  buf.delayTime = 71;
  buf.delayNum = 3;
  Receive(&src, (char *) in, sizeof(struct msg));
  Reply(src, (char *)&buf, sizeof(struct msg)); 
  Exit();
}
