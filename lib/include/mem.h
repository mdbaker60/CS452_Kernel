#ifndef _MEM_H_
#define _MEM_H_


char *memcpy(char *destination, const char *source, int num);
char *memcpy_aligned(char *destination, const char *source, int num);
char *memcpy_misaligned(char *destination, const char *source, int num);

#endif
