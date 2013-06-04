#include <syscall.h>
#include <string.h>
#include <bwio.h>
#include <memcpy.h>
#include <values.h>
#include <nameServer.h>

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
int retrieve(char* key,  entry* table);

int RegisterAs (char* task){
//-1 if the nameserver tid is invalid
//-2 if the nameserver tid is not the nameserver
//0 success
  struct msg snd;
  int ret;
  snd.control = 0;
  strcp(snd.message, task, strlen(task));
  Send(1, (char*)&snd, sizeof(struct msg), (char *) &ret, sizeof(int));	
  return 0;

}
int whoIs(char* task){
  struct msg snd;
  int ret;
  snd.control = 1;
  strcp(snd.message, task, strlen(task));
  Send(1, (char*)&snd, sizeof(struct msg), (char *) &ret, sizeof(int));	
  return ret;	
}

void NSInit(){
  int i;
  entry Table[100];
  for (i = 0; i < 100; i++){
    Table[i].name[0] = '\0';
  }

  int src;
  struct msg in;
  int  ret;
  while(true){
    Receive(&src, (char *)&in, sizeof(struct msg));
    switch (in.control){
      case 0:
	insert(in.message, src, Table);
	ret = 0;
	Reply(src, (char *)&ret, sizeof(int));
	break;
      case 1: 
	ret = retrieve(in.message, Table);
	Reply(src, (char *)&ret, sizeof(int));
	break;
    }	
  }
}

void insert(char* name, int TID, entry* table){
  int i = 0;
  while(table[i].name[0] != '\0'){//Look at later
    if (strcomp(table[i].name, name, strlen(name))){
      table[i].TID = TID;
      return;
    }
    ++i;
  }
  strcp(table[i].name, name, strlen(name));
  table[i].TID = TID;
}

int  retrieve(char* name, entry* table){
  int i = 0;
  while (table[i].name[0] != '\0'){
    if (strcomp(table[i].name, name, strlen(name))){
      return table[i].TID;
    }
  }
  return -1;
}

