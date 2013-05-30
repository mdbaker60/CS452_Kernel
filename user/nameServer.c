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
int retrieve(char* key,  entry* table);

int RegisterAs (char* task){
//-1 if the nameserver tid is invalid
//-2 if the nameserver tid is not the nameserver
//0 success
	bwprintf(COM2, "regas\r");
	struct msg snd;
	int ret;
	snd.control = 0;
	memcpy(snd.message, task, strlen(task));
	Send(1, (char*)&snd, sizeof(struct msg), (char *) &ret, sizeof(int));	
	bwprintf(COM2, "RETURNED: %d\r", ret);
	return 0;	

}
int whoIs(char* task){
	bwprintf(COM2, "WHOAMI: %s\r", task);
	struct msg snd;
	int ret;
	snd.control = 1;
	memcpy(snd.message, task, strlen(task));
	Send(1, (char*)&snd, sizeof(struct msg), (char *) &ret, sizeof(int));	
	bwprintf(COM2, "RETURNED: %d\r", ret);
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
	char msgbuf[sizeof(struct msg)];
	struct msg* in;
	int  ret;
	while(1){
		bwprintf(COM2, "NS:WAITING\r");
		Receive(&src, msgbuf, sizeof(struct msg));
		bwprintf(COM2, "NS:UP\r");
		in = (struct msg*)msgbuf;	
		bwprintf(COM2, "NS:RECEIVED: %d\r", in->control);
		switch (in->control){
		  case 0:
			insert(msgbuf, src, Table);
			Reply(src, "0", sizeof(2));
		
		case 1: 
			bwprintf(COM2, "Retreving: %s\r", in->message);
			ret = retrieve(msgbuf, Table);
			
			bwprintf(COM2, "Replying\r");
			Reply(src, (char *)&ret, sizeof(int));		
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
int  retrieve(char* name, entry* table){
	bwprintf(COM2, "IN ret\r");
	int i =0;
	while (table[i].name[0] != '\0'){
		if (strcomp(table[i].name, name, strlen(name)) == 0){
		bwprintf(COM2, "RETURNING\r");
		 return table[i].TID;
		}
	}
	bwprintf(COM2, "RETURNING2\r");
	return 0; //TODO fx me
}

