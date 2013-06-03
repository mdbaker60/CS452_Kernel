#include <bwio.h>
#include <syscall.h>
#include <nameServer.h>
#include <prng.h>
#include <clock.h>

struct msg{
	int control;
	int message;

};

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
	int tie = 3;
	int src, playRequest = -1;
	struct msg in;

	clockInit();
	seed(getClockTick());

	RegisterAs(who);
	while (1){
		Receive(&src, (char*) &in, sizeof(struct msg));
		switch(in.control){
		  case 0:
			if (playRequest == -1) playRequest = in.message;
			else{
				bwprintf(COM2, "\rTask %d playing against task %d\r\r", playRequest, in.message);
				Reply(playRequest, (char *)&win, sizeof(int));
				Reply(in.message, (char *)&win, sizeof(int));
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
			if(playerTable[src] == -1) {
				Reply(src, (char *)&quit, sizeof(int));
			}else if (playerMove[playerTable[src]] == -1){//Opponent has not moved yet
				playerMove[src] = in.message;
			}
			else{
				if (playerMove[playerTable[src]] == in.message){
					Reply(src, (char*)&tie, sizeof(int));
					Reply(playerTable[src], (char*)&tie, sizeof(int));
				}else if ((playerMove[playerTable[src]] == 2 && in.message == 0)|| (playerMove[playerTable[src]] == 1 && in.message == 2)
				   || (playerMove[playerTable[src]] == 0 && in.message == 1)){
					Reply(src, (char*)&win, sizeof(int));
					Reply(playerTable[src], (char*)&loss, sizeof(int));
				}else{
					Reply(src, (char *)&loss, sizeof(int));
					Reply(playerTable[src], (char *)&win, sizeof(int));
				}
				playerMove[playerTable[src]] = -1;
			}
			break;
		  case 2:
			if (playerTable[src] != -1){
				playerTable[playerTable[src]] = -1;
				Reply(src, (char *)&quit, sizeof(int));
				if(playerMove[playerTable[src]] != -1) {
					Reply(playerTable[src], (char *)&quit, sizeof(int));
				}
			}else{
				Reply(src, (char *)&quit, sizeof(int));
			}
		}
	}
}
void Client(){
	int RPSserver;
	int i;
	char* who = "RPS Server";
	struct msg out;
	int ret = -1;
	RPSserver = whoIs(who);
	out.control = 0;	//Tells the server we want to register
	out.message = MyTid(); 
	Send(RPSserver,  (char*)&out, sizeof(struct msg), (char *) &ret, sizeof(int));
	out.control = 1;
	for (i = 0; i < random() % 4 + 2; i++){
		//1 -	Win
		//0 -	Loss
		out.message = random() % 3;

		Send(RPSserver, (char*)&out, sizeof(struct msg), (char *) &ret, sizeof(int)); 
		if(ret == 2) break;
		switch(out.message) {
		  case 0:
			bwprintf(COM2, "%d: Playing rock\r", MyTid());
			break;
		  case 1:
			bwprintf(COM2, "%d: Playing paper\r", MyTid());
			break;
		  case 2:
			bwprintf(COM2, "%d: Playing Scissors\r", MyTid());
			break;
		}

		bwgetc(COM2);
		Pass();

		if (ret == 3) bwprintf(COM2, "%d: We Tie\r", MyTid());
		else if (ret) bwprintf(COM2, "%d: I Win\r", MyTid());
		else if (!ret) bwprintf(COM2, "%d: I Lose\r", MyTid());
		
		bwgetc(COM2);
	}
	out.control = 2;
	Send(RPSserver, (char*)&out, sizeof(struct msg), (char *) &ret, sizeof(int));
	Exit();
}
void firstTask() {
 // Create(0, producer);
  //Create(0, consumer);
  Create(7, NSInit);
  Create(6, Server);
  Create(5, Client);
  Create(5, Client);
  Create(5, Client);
  Create(5, Client);
  Exit();
}
