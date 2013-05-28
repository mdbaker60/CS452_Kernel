#include <mem.h>
#include <bwio.h>
void strcp(char* dest, char* src, int len){
	memcpy(dest, src, len);
}
int strlen(char* str){
	bwprintf(COM2, "In strlen\r");
	bwprintf(COM2, "str: %s", str);
	int ret = 0;

	while (str[ret] != '\0') ret++;
	return ret; 
}

int strcomp(char* str1, char* str2, int strlen){
	int i = 0;
	while (i < strlen && (str1[i] != '\0' && (str1[i] == str2[i]))) i++;
	return str1[i] - str2[i];
}


