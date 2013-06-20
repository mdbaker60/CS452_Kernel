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

  if(strcmp(command, "q") == 0) {
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

