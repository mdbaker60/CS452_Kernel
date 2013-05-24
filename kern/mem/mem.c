#include <mem.h>
#include <bwio.h>
static int allocs;
static int * memTop;
static struct memAlloc *bitMap;
void bitStrap(){
	//Initalize the bitMap
	bitMap = (struct memAlloc *) memTop;
	memTop -= (sizeof(struct memAlloc)) * 20000;	
	bitMap->memLoc = memTop;//The location of memory directly after the bitmap
	bitMap->size = 2000000;
}


void memInit(int *base){
	allocs = 0;
	//Given a pointer to the top of memory
	//need to allocate a pointer map, a large array
	//which holds pointers to contiguous blocks of memory
	//and their sizes
	
	//TODO predefine numerical constants
	//struct memAlloc bitMap[MAX_ALLOCS];
	memTop = base;
	bitStrap();

}

void * malloc(int size){
	//returns a pointer to a block of memory
	//0 if none could be found
	if (allocs == MAX_ALLOCS) 
		return (void *)0x1;
	
	int* ret = bitMap->memLoc;
	int test = bitMap->size;
	(bitMap+0) -> size = ((bitMap + allocs)->size) - size;
	(bitMap + allocs+0)->memLoc = ((bitMap + allocs)->memLoc) - size;
	allocs++;
	return ret - size;	
}
