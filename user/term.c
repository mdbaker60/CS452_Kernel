#include <term.h>
#include <string.h>
#include <syscall.h>
#include <io.h>
#include <values.h>

char *splitCommand(char *command) {
  int i=0;

  while(command[i] != '\0') {
    if(command[i] == ' ') {
      command[i] = '\0';
      return command + i + 1;
    }
    i++;
  }

  return command;
}

void parseCommand(char *command) {
  char *arg1 = splitCommand(command);
  char *arg2 = splitCommand(arg1);

  if(strcmp(command, "tr") == 0) {
    if(arg1 == arg2) {
      printf("\rError: command tr expects 2 arguments\r");
    }else{
      int trainNumber = strToInt(arg1);
      int trainSpeed = strToInt(arg2);
      if(trainNumber == -1 || trainNumber < 1 || trainNumber > 80) {
	printf("\rError: train number must be a number between 1 and 80\r");
      }else if(trainSpeed == -1 || trainSpeed < 0 || trainSpeed > 14) {
	printf("\rError: train speed must be a number between 0 and 14\r");
      }else {
        int trainCommand = (trainSpeed << 8) | trainNumber;
	sendTrainCommand(trainCommand);
      }
    }
  }else if(strcmp(command, "q") == 0) {
    Shutdown();
  }else{
    printf("\rUnrecognized command: \"%s\"\r", command);
  }
  Putc(2, '>');
}

void terminalDriver() {
  char curCommand[BUFFERSIZE];
  int commandLength = 0;
  char c;

  Putc(2, '>');
  while(true) {
    c = (char)Getc(2);
    switch(c) {
      case '\r':
	curCommand[commandLength] = '\0';
	parseCommand(curCommand);
	commandLength = 0;
	break;
      case '\x8':
	commandLength--;
	outputEscape("[1D[K");
	break;
      default:
	if(commandLength < BUFFERSIZE - 1) {
          curCommand[commandLength++] = c;
          Putc(2, c);
	}
        break;
    }
  }
}

