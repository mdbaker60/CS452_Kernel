#include <train.h>
#include <syscall.h>
#include <io.h>
#include <nameServer.h>
#include <values.h>
#include <clockServer.h>
#include <track.h>
#include <string.h>

struct TrainMessage {
  int type;
  int data[3];
};

struct TrainMessageBuf {
  int type;
  int data[3];
  char buffer[128];
};

struct TrainNode {
  int tid;
  int sensorNum;
  struct TrainNode *next;
  struct TrainNode *last;
};

void printSwitchTable(char *states) {
  int i;

  outputEscape("[s");
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
  moveCursor(4, 1);
  Putc(2, '|');
  for(i=1; i<19; i++) {
    printf("%c |", states[i-1]);
  }
  for(i=153; i<157; i++) {
    printf(" %c |", states[i-135]);
  }
  outputEscape("[u");
}

void printSensorList(int *history, int head) {
  int i, sensor;

  outputEscape("[s");
  moveCursor(6, 1);

  for(i=9; i>=0; i--) {
    sensor = history[(head+i)%10];
    if(sensor != ANYSENSOR) {
      printf("%c%d ", (char)(sensor/16)+65, sensor%16+1);
    }
  }

  outputEscape("[u");
}

void updateSensorInfo(struct SensorStates *oldStates, struct SensorStates *newStates, 
			struct SensorStates *bufferedSensors, int *history,
			int *head, struct TrainNode **first, struct TrainNode **last) {
  int i;
  for(i=0; i<NUMSENSORS; i++) {
    int old = getSensor(oldStates, i);
    int new = getSensor(newStates, i);
    if(old == 0 && new > 0) {
      history[(*head)++] = i;
      *head %= 10;

      struct TrainNode *cur = *first;
      while(cur != NULL) {
	if(cur->sensorNum == i || cur->sensorNum == ANYSENSOR) {
	  Reply(cur->tid, (char *)&i, sizeof(int));
	  if(cur->last == NULL && cur->next == NULL) {
	    *first = *last = NULL;
	  }else if(cur->last == NULL) {
	    *first = cur->next;
	    (cur->next)->last = NULL;
	  }else if(cur->next == NULL) {
	    *last = cur->last;
	    (cur->last)->next = NULL;
	  }else{
	    (cur->next)->last = cur->last;
	    (cur->last)->next = cur->next;
	  }
	  break;
	}
	cur = cur->next;
      }
      if(cur == NULL) {
	setSensor(bufferedSensors, i, true);
      }

      printSensorList(history, *head);
    }
  }

  oldStates->stateInfo[0] = newStates->stateInfo[0];
  oldStates->stateInfo[1] = newStates->stateInfo[1];
  oldStates->stateInfo[2] = newStates->stateInfo[2];
}

void sensorTask() {
  struct SensorStates states;

  while(true) {
    getSensorData(&states);
    setSensorData(&states);

    Delay(1);
  }
}

void changeSwitch(int switchNum, char state) {
  if(state == 'S') {
    Putc2(1, (char)33, (char)switchNum);
  }else if(state == 'C') {
    Putc2(1, (char)34, (char)switchNum);
  }
  Putc(1, (char)32);
}

void TrainInit() {
  int i;
  struct SensorStates sensorStates;
  struct SensorStates bufferedSensors;
  char switchStates[NUMSWITCHES];
  for(i=0; i<NUMSWITCHES; i++) {
    switchStates[i] = '?';
  }
  int sensorHistory[10];
  for(i=0; i<10; i++) {
    sensorHistory[i] = ANYSENSOR;
  }
  int historyHead = 0;
  int min=0, sec=0, tenthSec=0;
  int activeTrack = TRACKA;

  while(whoIs("Output Server") == -1) Pass();
  getSensorData(&sensorStates);
  Create(1, sensorTask);

  struct TrainNode nodes[100];
  struct TrainNode *first = NULL;
  struct TrainNode *last = NULL;
  RegisterAs("Train Server");

  int src, reply;
  struct TrainMessageBuf msg;
  struct SensorStates newStates;
  struct TrainNode *tempNode;
  while(true) {
    Receive(&src, (char *)&msg, sizeof(struct TrainMessageBuf));
    switch(msg.type) {
      case TRAINPRINTCLOCK:
	min = msg.data[0];
	sec = msg.data[1];
	tenthSec = msg.data[2];
	printAt(1, 1, "%d%d:%d%d:%d", min/10, min%10, sec/10, sec%10, tenthSec);
	reply = 0;
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINSETSWITCH:
	if(msg.data[0] <= 18 && switchStates[msg.data[0]-1] != (char)msg.data[1]) {
	  switchStates[msg.data[0]-1] = (char)msg.data[1];
	  printAt(4, (msg.data[0]*3)-1, "%c", (char)msg.data[1]);
	  changeSwitch(msg.data[0], (char)msg.data[1]);
	}else if(msg.data[0] >= 153 && switchStates[msg.data[0]-135] != (char)msg.data[1]) {
	  switchStates[msg.data[0]-135] = (char)msg.data[1];
	  printAt(4, ((msg.data[0]-152)*4)+53, "%c", (char)msg.data[1]);
	  changeSwitch(msg.data[0], (char)msg.data[1]);
	}

	reply = 0;
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINGETSWITCH:
	if(msg.data[0] <= 18) {
		reply = switchStates[msg.data[0]-1];
	}else{
		reply = switchStates[msg.data[0]-135];
	}
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINBLOCKSENSOR:
	if(getSensor(&bufferedSensors, msg.data[0]) == true) {
	  setSensor(&bufferedSensors, msg.data[0], false);
	  Reply(src, (char *)&reply, sizeof(int));
	  break;
	}
	tempNode = &nodes[src & 0x7F];
	tempNode->tid = src;
	tempNode->sensorNum = msg.data[0];
	tempNode->next = NULL;
	if(first == NULL) {
		tempNode->last = NULL;
		first = last = tempNode;
	}else{
		tempNode->last = last;
		last->next = tempNode;
		last = tempNode;
	}
	break;
      case TRAINSETSENSORS:
	newStates.stateInfo[0] = msg.data[0];
	newStates.stateInfo[1] = msg.data[1];
	newStates.stateInfo[2] = msg.data[2];
	updateSensorInfo(&sensorStates, &newStates, &bufferedSensors, sensorHistory, &historyHead, &first, &last);
	reply = 0;
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINPRINTDEBUG:
	outputEscape("[s");
	moveCursor(7+msg.data[0], 1);
	outputEscape("[K");
	printString(2, msg.buffer);
	outputEscape("[u");
	reply = 0;
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINREFRESHSCREEN:
	printAt(1, 1, "%d%d:%d%d:%d", min/10, min%10, sec/10, sec%10, tenthSec);
	printSwitchTable(switchStates);
	printSensorList(sensorHistory, historyHead);
	clearLine(2);
	clearLine(5);
	clearLine(7);
	clearLine(8);
	clearLine(9);
	clearLine(10);
	clearLine(12);
	reply = 0;
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINSETTRACK:
	activeTrack = msg.data[0];
	reply = 0;
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINGETTRACK:
	reply = activeTrack;
	Reply(src, (char *)&reply, sizeof(int));
	break;
    }
  }
}

void printTime(int min, int sec, int tenthSec) {
	struct TrainMessage msg;
	msg.type = TRAINPRINTCLOCK;
	msg.data[0] = min;
	msg.data[1] = sec;
	msg.data[2] = tenthSec;
	int reply;

	Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

void setSwitchState(int switchNum, char state) {
  struct TrainMessage msg;
  msg.type = TRAINSETSWITCH;
  msg.data[0] = switchNum;
  msg.data[1] = (int)state;
  int reply;

  int server = whoIs("Train Server");
  while(server == -1) server = whoIs("Train Server");

  Send(server, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

char getSwitchState(int switchNum) {
  struct TrainMessage msg;
  msg.type = TRAINGETSWITCH;
  msg.data[0] =  switchNum;
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));

  return (char)reply;
}

void waitOnSensor(int sensorNum) {
  struct TrainMessage msg;
  msg.type = TRAINBLOCKSENSOR;
  msg.data[0] = sensorNum;
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

int waitOnAnySensor() {
  struct TrainMessage msg;
  msg.type = TRAINBLOCKSENSOR;
  msg.data[0] = ANYSENSOR;
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));

  return reply;
}

void setSensorData(struct SensorStates *states) {
  struct TrainMessage msg;
  msg.type = TRAINSETSENSORS;
  msg.data[0] = states->stateInfo[0];
  msg.data[1] = states->stateInfo[1];
  msg.data[2] = states->stateInfo[2];
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

void printDebugInfo(int line, char *debugInfo) {
  struct TrainMessageBuf msg;
  msg.type = TRAINPRINTDEBUG;
  msg.data[0] = line;
  strcpy(msg.buffer, debugInfo);
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessageBuf), (char *)&reply, sizeof(int));
}

void refreshScreen() {
  struct TrainMessage msg;
  msg.type = TRAINREFRESHSCREEN;
  int reply;

  int server = whoIs("Train Server");
  while(server == -1) {
    server = whoIs("Train Server");
  }

  Send(server, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

void setTrack(int track) {
  struct TrainMessage msg;
  msg.type = TRAINSETTRACK;
  msg.data[0] = track;
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

int getTrack() {
  struct TrainMessage msg;
  msg.type = TRAINGETTRACK;
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));

  return reply;
}

void initTrack(track_node *track) {
  int trackID = getTrack();

  switch(trackID) {
    case TRACKA:
      init_tracka(track);
      break;
    case TRACKB:
      init_trackb(track);
      break;
  }
}




void setSensor(struct SensorStates *states, int sensorNum, int state) {
  int word = states->stateInfo[sensorNum/32];
  int offset = sensorNum % 32;
  if(state) {
    word |= 0x80000000 >> offset;
  }else{
    word &= ~(0x80000000 >> offset);
  }

  states->stateInfo[sensorNum/32] = word;
}

int getSensor(struct SensorStates *states, int sensorNum) {
  int word = states->stateInfo[sensorNum/32];
  int offset = sensorNum % 32;
  if(word & (0x80000000 >> offset)) {
    return true;
  }else{
    return false;
  }
}

int setSensorByte(struct SensorStates *states, int byteNum, int byte) {
  int word = states->stateInfo[byteNum/4];
  int byteOffset = 24 - 8*(byteNum % 4);
  word &= ~(0x000000FF << byteOffset);
  word |= (byte << byteOffset);

  states->stateInfo[byteNum/4] = word;

  return 0;
}



void getSensorData(struct SensorStates *states) {
  int i, sensorByte, byte;

  Putc(1, (char)133);
  for(i=0; i<10; i++) {
    sensorByte = Getc(1);
    setSensorByte(states, i, sensorByte);
  }
}
