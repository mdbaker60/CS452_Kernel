#include <syscall.h>
#include <string.h>
#include <bwio.h>
#include <memcpy.h>
#include <values.h>
#include <nameServer.h>

struct NSMessage {
  int type;
  char message[100];
};
//typedef struct e_struct entry;

/*struct e_struct
{
  char name[100];
  int TID;
};*/

struct BucketEntry {
  char name[100];
  int TID;
  struct BucketEntry *next;
};

struct Bucket {
  struct BucketEntry *first;
  struct BucketEntry *last;
};

struct HashTable {
  struct Bucket buckets[128];
  struct BucketEntry entries[200];
  int nextFreeEntry;
};

void insert(char *name, int TID, struct HashTable *table);
int retrieve(char *name, struct HashTable *table);
int getHashValue(char *key);

int RegisterAs(char *task) {
//-1 if the nameserver tid is invalid
//-2 if the nameserver tid is not the nameserver
//0 success
  struct NSMessage snd;
  int ret;
  snd.type = NSREGISTER;
  strcpy(snd.message, task);
  Send(1, (char*)&snd, sizeof(struct NSMessage), (char *) &ret, sizeof(int));	
  return 0;

}

int whoIs(char* task){
  struct NSMessage snd;
  int ret;
  snd.type = NSWHOIS;
  strcpy(snd.message, task);
  Send(1, (char*)&snd, sizeof(struct NSMessage), (char *) &ret, sizeof(int));	
  return ret;	
}

void NSInit(){
  int i;
  struct HashTable table;
  for(i=0; i<128; i++) {
    table.buckets[i].first = table.buckets[i].last = NULL;
  }
  table.nextFreeEntry = 0;

  int src;
  struct NSMessage in;
  int reply;
  while(true) {
    Receive(&src, (char *)&in, sizeof(struct NSMessage));
    switch(in.type) {
      case NSREGISTER:
	insert(in.message, src, &table);
	reply = 0;
	Reply(src, (char*)&reply, sizeof(int));
	break;
      case NSWHOIS:
	reply = retrieve(in.message, &table);
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case NSSHUTDOWN:
	Reply(src, (char *)&reply, sizeof(int));
	Destroy(MyTid());
	break;
    }
  }
}

/*  int i;
  entry Table[100];
  for (i = 0; i < 100; i++){
    Table[i].name[0] = '\0';
  }

  int src;
  struct NSMessage in;
  int  ret;
  while(true){
    Receive(&src, (char *)&in, sizeof(struct NSMessage));
    switch (in.type){
      case NSREGISTER:
	insert(in.message, src, Table);
	ret = 0;
	Reply(src, (char *)&ret, sizeof(int));
	break;
      case NSWHOIS:
	ret = retrieve(in.message, Table);
	Reply(src, (char *)&ret, sizeof(int));
	break;
      case NSSHUTDOWN:
	Reply(src, (char *)&ret, sizeof(int));
	int tid = MyTid();
	Destroy(tid);
	break;
    }	
  }
}*/

void insert(char *name, int TID, struct HashTable *table) {
  int hash = getHashValue(name) & 0x7F;
  struct BucketEntry *entry = (table->buckets[hash]).first;
  //scan for the key in the bucket, and replace if there
  while(entry != NULL && strcmp(entry->name, name) != 0) {
    entry = entry->next;
  }

  if(entry == NULL) {
    struct BucketEntry *newEntry = &(table->entries[(table->nextFreeEntry)++]);
    newEntry->next = NULL;
    newEntry->TID = TID;
    strcpy(newEntry->name, name);
    if((table->buckets[hash]).first == NULL) {
      (table->buckets[hash]).first = (table->buckets[hash]).last = newEntry;
    }else{
      ((table->buckets[hash]).last)->next = newEntry;
      (table->buckets[hash]).last = newEntry;
    }
  }else{
    entry->TID = TID;
  }
}

/*void insert(char* name, int TID, entry* table){
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
}*/

int retrieve(char *name, struct HashTable *table) {
  int hash = getHashValue(name) & 0x7F;
  struct BucketEntry *entry = (table->buckets[hash]).first;
  while(entry != NULL) {
    if(strcmp(entry->name, name) == 0) {
      return entry->TID;
    }
    entry = entry->next;
  }

  return -1;
}

/*int  retrieve(char* name, entry* table){
  int i = 0;
  while (table[i].name[0] != '\0'){
    if (strcomp(table[i].name, name, strlen(name))){
      return table[i].TID;
    }
  }
  return -1;
}*/

int getHashValue(char *key) {
  int hash = 0;
  while(*key != '\0') {
    hash = ((hash << 5) + hash) + (int)*key;
    key++;
  }

  return hash;
}
