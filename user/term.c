#include <bwio.h>
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

struct trainInfo {
  int speed;
  int number;
};

void reverser() {
  int reply = 0, src;
  struct trainInfo info;
  Receive(&src, &info, sizeof(struct trainInfo));
  Reply(src, &reply, sizeof(int));

  Delay(150);
  //reverse command
  int trainCommand = (15 << 8) | info.number;
  sendTrainCommand(trainCommand);
  //back to speed command
  trainCommand = (info.speed << 8) | info.number;
  sendTrainCommand(trainCommand);

  Destroy(MyTid());
}

void parseCommand(char *command, int *trainSpeeds) {
  char *arg1 = splitCommand(command);
  char *arg2 = splitCommand(arg1);

  if(strcmp(command, "tr") == 0) {
    if(arg1 == arg2) {
      printf("\rError: command tr expects 2 arguments");
    }else{
      int trainNumber = strToInt(arg1);
      int trainSpeed = strToInt(arg2);
      if(trainNumber == -1 || trainNumber < 1 || trainNumber > 80) {
	printf("\rError: train number must be a number between 1 and 80");
      }else if(trainSpeed == -1 || trainSpeed < 0 || trainSpeed > 14) {
	printf("\rError: train speed must be a number between 0 and 14");
      }else {
        int trainCommand = (trainSpeed << 8) | trainNumber;
	sendTrainCommand(trainCommand);
	trainSpeeds[trainNumber] = trainSpeed;
      }
    }
  }else if(strcmp(command, "rv") == 0) {
    if(arg1 == command) {
      printf("\rError: command rv expects an argument");
    }else{
      int trainNumber = strToInt(arg1);
      if(trainNumber == -1 || trainNumber < 1 || trainNumber > 80) {
	printf("\rError: train number must be a number between 1 and 80");
      }else{
	sendTrainCommand(trainNumber);
	int reverseTask = Create(1, reverser);
        int reply;
	struct trainInfo info;
	info.speed = trainSpeeds[trainNumber];
	info.number = trainNumber;
	Send(reverseTask, &info, sizeof(struct trainInfo), &reply, sizeof(int));
      }
    }
  }else if(strcmp(command, "sw") == 0) {
    if(arg1 == arg2) {
      printf("\rError: command sw expects two arguments");
    }else{
      int switchNumber = strToInt(arg1);
      char switchDirection = arg2[0];
      if(switchDirection != 'S' && switchDirection != 'C') {
	printf("\rError: switch direction must be 'S' or 'C'");
      }else{
	Putc(1, (char)33);
	Putc(1, (char)switchNumber);
	Putc(1, (char)32);
      }
    }
  }else if(strcmp(command, "q") == 0) {
    Shutdown();
  }else{
    printf("\rUnrecognized command: \"%s\"", command);
  }
  printf("\r>");
}

void terminalDriver() {
  char commands[NUM_SAVED_COMMANDS][BUFFERSIZE];
  int lengths[NUM_SAVED_COMMANDS];
  int commandNum = 0;

  char *curCommand = commands[0];
  int commandLength = 0;

  char c;

  int trainSpeeds[80];
  int i;
  for(i=0; i<80; i++) {
    trainSpeeds[i] = 0;
  }

  Putc(2, '>');
  while(true) {
    c = (char)Getc(2);
    switch(c) {
      case '\r':
	curCommand[commandLength] = '\0';
	parseCommand(curCommand, trainSpeeds);
	lengths[commandNum] = commandLength;
	commandNum++;
	commandNum %= NUM_SAVED_COMMANDS;
	curCommand = commands[commandNum];
	commandLength = 0;
	break;
      case '\x8':
	if(commandLength > 0) {
	  commandLength--;
	  outputEscape("[1D[K");
	}
	break;
      case '\x1B':	//escape
	c = (char)Getc(2);
	c = (char)Getc(2);
	if(c == 'A') {
	  lengths[commandNum] = commandLength;
	  commandNum += NUM_SAVED_COMMANDS - 1;
	  commandNum %= NUM_SAVED_COMMANDS;
	  curCommand = commands[commandNum];
	  commandLength = lengths[commandNum];
	  outputEscape("[100D[K");
	  printf(">%s", curCommand);
	}else if(c == 'B') {
	  printf("down");
	}else if(c == 'C') {
	  printf("right");
	}else if(c == 'D') {
	  printf("down");
	}
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

