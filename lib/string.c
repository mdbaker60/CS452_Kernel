#include <memcpy.h>
#include <bwio.h>
#include <string.h>
//void strcp(char* dest, char* src, int len){
//	memcpy(dest, src, len+1);
//}
char *strcpy(char *destination, const char *source) {
  int length = strlen((char *)source)+1;

  if(length >= 4) {
    return memcpy(destination, source, strlen((char *)source)+1);
  }

  int i=0;
  while(source[i] != '\0') {
    destination[i] = source[i];
    i++;
  }
  destination[i] = '\0';
  return destination;
}

int strlen(char* str){
	int ret = 0;

	while (str[ret] != '\0') ret++;
	return ret;
}

//int strcomp(char* str1, char* str2, int strlen){
//	int i = 0;
//	while (i < strlen && (str1[i] != '\0' && (str1[i] == str2[i]))) i++;
//	return !(str1[i] - str2[i]);
//}
int strcmp(const char *str1, const char *str2) {
  int i=0;
  while(str1[i] != '\0' && str1[i] == str2[i]) i++;
  return str1[i] - str2[i];
}


int strToInt(char *string) {
  int base = 10;
  int retVal = 0;

  while(*string != '\0') {
    int digit = (int)*string - 48;
    if(digit < 0 || digit > 9) {
      return -1;
    }
    retVal *= base;
    retVal += digit;
    string++;
  }

  return retVal;
}
