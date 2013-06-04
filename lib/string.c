#include <memcpy.h>
#include <bwio.h>
void strcp(char* dest, char* src, int len){
	memcpy(dest, src, len+1);
}
int strlen(char* str){
	int ret = 0;

	while (str[ret] != '\0') ret++;
	return ret+1; 
}

int strcomp(char* str1, char* str2, int strlen){
	int i = 0;
	while (i < strlen && (str1[i] != '\0' && (str1[i] == str2[i]))) i++;
	return str1[i] - str2[i];
}


