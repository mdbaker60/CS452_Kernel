#ifndef _NAMESERVER_H_
#define _NAMESERVER_H_

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


void NSInit();
int whoIs(char* task);
int RegisterAs(char* name);
void insert(char * key, int idx, entry* table);
int retrieve(char* key,  entry* table);

#endif
