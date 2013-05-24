#ifndef __MEM_H__
#define __MEM_H__
#define MAX_ALLOCS 	20000
#define MAX_MEM 	2000000
struct memAlloc{
	int* memLoc;
	int size;	//Size in bytes
	char data;	//If memory is in use

};

void memInit(int *base);		//Initalizes the memory map

void *malloc(int size);
#endif
