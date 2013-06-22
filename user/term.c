#include <bwio.h>
#include <term.h>
#include <string.h>
#include <syscall.h>
#include <io.h>
#include <values.h>
#include <clockServer.h>

void updateSwitchTable(int switchNum, char state);
void initSwitchTable();
int saveSensor(char *buffer, int *head, int data, int byteNumber, int *states);
void printSensorList(char *buffer, int head);

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
  Receive(&src, (char *)&info, sizeof(struct trainInfo));
  Reply(src, (char *)&reply, sizeof(int));

  Delay(150);
  //reverse command
  int trainCommand = (15 << 8) | info.number;
  sendTrainCommand(trainCommand);
  //back to speed command
  trainCommand = (info.speed << 8) | info.number;
  sendTrainCommand(trainCommand);

  Destroy(MyTid());
}

void clockDriver() {
  int time = Time(), tenthSecs = 0, secs = 0, mins = 0;

  while(true) {
    time += 10;
    DelayUntil(time);
    tenthSecs++;
    if(tenthSecs == 10) {
      tenthSecs = 0;
      secs++;
      if(secs == 60) {
	secs = 0;
	mins++;
      }
    }
    printAt(1, 1, "%d%d:%d%d:%d", mins/10, mins%10, secs/10, secs%10, tenthSecs);
  }
}

void sensorDriver() {
  int i;
  int sensorStates[80];
  for(i=0; i<80; i++) {
    sensorStates[i] = false;
  }

  char sensorBuffer[30];
  for(i=0; i<30; i++) {
    sensorBuffer[i] = ' ';
  }
  int bufHead = 0;

  int sensorData, listUpdated;
  while(true) {
    listUpdated = false;
    Putc(1, (char)133);
    for(i=0; i<10; i++) {
      sensorData = Getc(1);
      listUpdated |= saveSensor(sensorBuffer, &bufHead, sensorData, i, sensorStates);
    }
    if(listUpdated) {
      printSensorList(sensorBuffer, bufHead);
    }
    Delay(10);
  }
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
	Send(reverseTask, (char *)&info, sizeof(struct trainInfo), (char *)&reply, sizeof(int));
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
	updateSwitchTable(switchNumber, switchDirection);
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

  Create(2, clockDriver);
  Create(2, sensorDriver);

  outputEscape("[2J");
  initSwitchTable();
  moveCursor(8, 1);
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
	  printf("left");
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

void updateSwitchTable(int switchNum, char state) {
  int colChar;
  if(switchNum > 18) {
    colChar = ((switchNum-152)*4)+53;
  }else{
    colChar = (switchNum*3)-1;
  }
  outputEscape("[s[4;");
  printInt(2, colChar, 10);
  Putc(2, 'f');
  Putc(2, state);
  outputEscape("[u");
}

void initSwitchTable() {
  int i;

  moveCursor(3,1);
  Putc(2, '|');
  for(i=1; i<19; i++) {
    printf("%d%d", i/10, i%10);
    Putc(2, '|');
  }
  for(i=153; i<157; i++) {
    printInt(2, i, 10);
    Putc(2, '|');
  }
  moveCursor(4,1);
  Putc(2, '|');
  for(i=1; i<19; i++) {
    printf("? |");
  }
  for(i=153; i<157; i++) {
    printf(" ? |");
  }
}

int saveSensor(char *buffer, int *head, int data, int byteNumber, int *states) {
  int moduleNum = (byteNumber/2);
  int i, stateChanged = false;

  for(i=0; i<8; i++) {
    int sensorNum = 8-i;
    if(byteNumber%2) sensorNum += 8;

    if(data%2 == 1 && states[moduleNum*16 + sensorNum - 1] == false) {
      states[moduleNum*16 + sensorNum - 1] = true;
      *head += 29;
      *head %= 30;
      buffer[*head] = (char)(sensorNum%10)+48;
      *head += 29;
      *head %= 30;
      buffer[*head] = (char)(sensorNum/10)+48;
      *head += 29;
      *head %= 30;
      buffer[*head] = (char)moduleNum+65;
      stateChanged = true;
    }else if(data%2 == 0 && states[moduleNum*16 + sensorNum - 1] == true) {
      states[moduleNum*16 + sensorNum - 1] = false;
      stateChanged = true;
    }

    data /= 2;
  }

  return stateChanged;
}

void printSensorList(char *buffer, int head) {
  int i;

  for(i=0; i<10; i++) {
    printAt(6, i*4 + 1, "%c%c%c", buffer[head], buffer[head+1], buffer[head+2]);
    head += 3;
    head %= 30;
  }
}
