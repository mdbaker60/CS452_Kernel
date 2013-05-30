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
	char message[100];
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
	bwprintf(COM2, "Registering Server: %s\r", task);
	struct msg snd;
	int ret;
	snd.control = 0;
	memcpy(snd.message, task, strlen(task));
	Send(1, (char*)&snd, sizeof(struct msg), (char *) &ret, sizeof(int));	
	bwprintf(COM2, "RegisterAs: RETURNED: %c\r", ret);
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
	return ret;	
}

void NSInit(){
	bwprintf(COM2, "STARTING NS: %d\r", MyTid());
	int i;
	entry Table[100];
	for (i = 0; i < 100; i++){
		Table[i].name[0] = '\0';
	}
	int src;
	struct msg in;
	int  ret;
	while(1){
		bwprintf(COM2, "NS:WAITING\r");
		Receive(&src, (char *) &in, sizeof(struct msg));
		switch (in.control){
		  case 0:
			bwprintf(COM2, "NS:Inserting: %s\r", in.message);
			insert(in.message, src, Table);
			bwprintf(COM2, "NS:Inserted\r");
			Reply(src, "0", sizeof(2));
			break;
		  case 1: 
			bwprintf(COM2, "NS:Retreving: %s\r", in.message);
			ret = retrieve(in.message, Table);
			bwprintf(COM2, "NS:Replying: %d\r", ret);
			Reply(src, (char *)&ret, sizeof(int));		
		}	
	}
}
void insert(char* name, int TID, entry* table){
	int i = 0;
	bwprintf(COM2, "Inserting %s\r", name);
	while(table[i].name[0] != '\0'){//Look at later
		if (strcomp(table[i].name, name, strlen(name)) == 0){
			table[i].TID = TID;
			return;
		}
		i++;
	}
	strcp(table[i].name, name, strlen(name));
	table[i].TID = TID;
}
int  retrieve(char* name, entry* table){
	bwprintf(COM2, "Retrieving: %s\r", name);
	int i = 0;
	while (table[i].name[0] != '\0'){
		if (strcomp(table[i].name, name, strlen(name)) == 0){
		bwprintf(COM2, "RETURNING: %d\r", table[i].TID);
		 return table[i].TID;
		}
	}
	bwprintf(COM2, "RETURNING: -1\r");
	return -1;
}

