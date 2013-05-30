#include <syscall.h>
#include <string.h>
#include <bwio.h>
#include <mem.h>
#define FNV_PRIME 16777619
#define FNV_OFFSET_BASIS 2166136261u
//TODO, place memcpy into string function
//Reply with tid on WhoIs
//Reply with 0 on successful insert
//TODO seperate HASH implmentation into lib
struct msg
{
	int control;
	char* message;
};
typedef struct e_struct entry;

struct e_struct
{
	char name[100];
	int TID;	
};
void insert(char * key, int idx, entry* table);
int hash(char * key);
int * retrieve(char* key,  entry* table);

int RegisterAs (char* task){
//-1 if the nameserver tid is invalid
//-2 if the nameserver tid is not the nameserver
//0 success
	int len = strlen(task); //does NOT include null character
	char ret[100];
	struct msg snd;
	snd.control =0;
	memcpy(snd.message, task, len);
	bwprintf(COM2, "REGGING: %s\r", task);
	snd.message[len] = '\0';
	//Makesure buffer recieve size is properly set
	bwprintf(COM2, "Contents: %d, %s \r", snd.control, snd.message);
	bwprintf(COM2, "SENDING\r");
	Send(1, (char*) &snd, sizeof(struct msg), (char *) &ret, 50); 
	return 0;	

}
int whoIs(char* task){
	bwprintf(COM2, "WHOAMI: %s\r", task);
	struct msg snd;
	int ret;
	snd.control = 0;
	memcpy(snd.message, task, strlen(task));
	snd.message = task;
	Send(1, (char*)&snd, sizeof(struct msg), (char *) &ret, sizeof(int));	
return 0;	
}

void NSInit(){
	bwprintf(COM2, "STARTING NS: %d\r", MyTid());
	int i;
	entry Table[100];
	for (i = 0; i < 100; i++){
		Table[i].name[0] = '\0';
	}
	int src;
	char msgbuf[104];
	struct msg* in;
	int * ret;
	while(1){
		bwprintf(COM2, "WAITING\r");
		Receive(&src, msgbuf, sizeof(struct msg));
		bwprintf(COM2, "UP\r");
		bwprintf(COM2, "RECEIVED: %d\r", in->control);
		in = (struct msg*)msgbuf;	
		switch (in->control){
		  case 0:
			insert(msgbuf, src, Table);
			Reply(src, "0", sizeof(2));
		
		case 1: 
			ret = retrieve(msgbuf, Table);
			Reply(src, (char *)ret, sizeof(int));		
		}	
	}
}
void insert(char* name, int TID, entry* table){
	int i = 0;
	while(table[i].name != '\0'){
		if (strcomp(table[i].name, name, strlen(name)) == 0){
			table[i].TID = TID;
			return;
		}
		i++;
	}
	strcp(table[i].name, name, strlen(name));
}
int * retrieve(char* name, entry* table){
	int i =0;
	while (table[i].name[0] != '\0'){
		if (strcomp(table[i].name, name, strlen(name)) == 0) return &table[i].TID;
	}
	return -1; //TODO fx me
}

