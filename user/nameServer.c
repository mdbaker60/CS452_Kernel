#include <syscall.h>
#include <string.h>
#include <bwio.h>
//TODO, place memcpy into string function
//Reply with tid on WhoIs
//Reply with 0 on successful insert
void insert(int src, char * key);
int hash(char * key);
int retrieve(char* key);
static char* values[100];
int mode = 0;

int whoIs(char* task, char* rep){
	bwprintf(COM2, "whoIs-Request for: %s\r", task);
	bwprintf(COM2, "strlen: %d\r", strlen(task));
	mode = 0;
	Send(1, task, strlen(task), rep, 3);	
return 0;	
}



void NSInit(){
	bwprintf(COM2, "STARTING NS\r");
	int i;
	char* buf;
	for (i = 0; i < 100; i++){
		//TODO: Dynamic?
		values[i] = &buf[104];
	}

	int *src;
	char msgbuf[104];
	for (i = 0; i < 100; i++){
		values[i] = "\0";
	}
	while(1){
		Receive(src, msgbuf, 100);
		bwprintf(COM2, "AFTER RECIEVE\r");
		hash(msgbuf);
		if (mode == 1){
			insert(*src, msgbuf);
			Reply(*src, "0", 1);
		}
		else if (mode == 0){
			char ret = (char) retrieve(msgbuf);
			Reply(*src, &ret, sizeof(int));
		}		
		
	}	
}


void insert(int src, char* key){
	strcp(key, values[src], 100);
}

int retrieve(char* key){
	//TODO need to compare strings within the values array
	return values[hash(key)];
} 

int hash(char* key){
	return 0;	
}
