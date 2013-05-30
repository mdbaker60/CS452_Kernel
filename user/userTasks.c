#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>
struct msg{
	int control;
	int message;

};


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

void Server(){
	int j;
	int playerTable[100];
	int playerMove[100];
	for (j = 0; j < 100; j++){
		playerMove[j] = -1;
	}
	char* who = "RPS Server";
	int win = 1;
	int loss = 0;
	int quit = 2;
	int src, playRequest = -1;
	struct msg in;
	bwprintf(COM2, "Starting server\r");
	RegisterAs(who);
	bwprintf(COM2, "PASSING:\r");		
	while (1){
		Receive(&src, (char*) &in, sizeof(struct msg));
		switch(in.control){
		  case 0:
		  	bwprintf(COM2, "RPSServer: Register\r");
			if (playRequest == -1) playRequest = in.message;
			else{
				Reply(playRequest, 0, sizeof(int));
				Reply(in.message, 0, sizeof(int));
				bwprintf(COM2, "RPSServer: Matching tids %d:%d", playRequest, in.message);
				playerTable[playRequest] = in.message;
				playerTable[in.message] = playRequest;
				playRequest = -1;
			}
		  	break;
		  case 1:
			//in.message contains an int representing R,P or S
			//	R:0
			//	P:1
			//	S:2
			bwprintf(COM2, "RPSServer: Play\r");
			if (playerMove[playerTable[src]] == -1){//Opponent has not moved yet
				playerMove[src] = in.message;
			}
			else{
				if ((playerMove[src] == 2 && in.message == 0)|| in.message > playerMoves[src]){
					Reply(in.message, (char*)&win, sizeof(int));
					Reply(playerMove[src], (char*)&loss, sizeof(int));
				}
				else{
					
					Reply(in.message, (char *)&loss, sizeof(int));
					Reply(playerMove[src], (char *)&win, sizeof(int));
				}
				playerMoves[src] = -1;	
			}
			break;
		  case 2:
			bwprintf(COM2, "RPSServer: Quit\r");
			Send(playerTable[src], &quit, sizeof(int));
		}
	}
}
void Client(){
	int RPSserver;
	int i;
	char* who = "RPS Server";
	struct msg out;
	int ret;
	bwprintf(COM2, "Starting client\r");
	RPSserver = whoIs(who);
	out.control = 0;	//Tells the server we want to register
	out.message = MyTid(); 
	Send(RPSserver,  (char*)&out, sizeof(struct msg), (char *) &ret, sizeof(int));
	out.control = 1;
	int move = 0;
	for (i = 0; i < 12; i++){
		//1 -	Win
		//0 -	Loss
		out.message = move;	
		Send(RPSserver, (char*)&out, sizeof(struct msg), (char *) &ret, sizeof(int)); 

	}
	out.control = 2;
	Send(RPSserver, (char*)&out, sizeof(struct msg), (char *) &ret, sizeof(int));
	
}
void firstTask() {
 // Create(0, producer);
  //Create(0, consumer);
  Create(2, NSInit);
  Create(5, Server);
  Create(5, Client);
  Pass();
  Exit();
}
