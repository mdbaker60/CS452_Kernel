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
  int tid;
};

struct TrainMessageBuf {
  int type;
  int data[3];
  int tid;
  char buffer[128];
};

struct TrainNode {
  int tid;
  struct SensorStates sensors;
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
			int *history, int *head, struct TrainNode **first, struct TrainNode **last) {
  int i;
  for(i=0; i<NUMSENSORS; i++) {
    int old = getSensor(oldStates, i);
    int new = getSensor(newStates, i);
    if(old == 0 && new > 0) {
      history[(*head)++] = i;
      *head %= 10;

      struct TrainNode *cur = *first;
      while(cur != NULL) {
	if(getSensor(&(cur->sensors), i)) {
	  int replyVal = Reply(cur->tid, (char *)&i, sizeof(int));
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
	  if(replyVal >= 0) break;
	}
	cur = cur->next;
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
      case TRACKPRINTCLOCK:
	min = msg.data[0];
	sec = msg.data[1];
	tenthSec = msg.data[2];
	printAt(1, 1, "%d%d:%d%d:%d", min/10, min%10, sec/10, sec%10, tenthSec);
	reply = 0;
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRACKSETSWITCH:
	if(msg.data[0] < 0 || msg.data[0] > 156 || (msg.data[0] > 18 && msg.data[0] < 153)) {
	  printColored(RED, BLACK, "ERROR: train server given invalid switch number %d\r", msg.data[0]);
	}else if(msg.data[0] <= 18 && switchStates[msg.data[0]-1] != (char)msg.data[1]) {
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
      case TRACKGETSWITCH:
	if(msg.data[0] <= 18) {
	  reply = switchStates[msg.data[0]-1];
	}else{
	  reply = switchStates[msg.data[0]-135];
	}
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRACKBLOCKSENSOR:
	tempNode = &nodes[src & 0x7F];
	tempNode->tid = src;
	(tempNode->sensors).stateInfo[0] = msg.data[0];
	(tempNode->sensors).stateInfo[1] = msg.data[1];
	(tempNode->sensors).stateInfo[2] = msg.data[2];

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
      case TRACKSETSENSORS:
	newStates.stateInfo[0] = msg.data[0];
	newStates.stateInfo[1] = msg.data[1];
	newStates.stateInfo[2] = msg.data[2];
	updateSensorInfo(&sensorStates, &newStates, sensorHistory, &historyHead, &first, &last);
	reply = 0;
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRACKPRINTDEBUG:
	outputEscape("[s");
	moveCursor(7+msg.data[0], 1);
	outputEscape("[K");
	printString(2, msg.buffer);
	outputEscape("[u");
	reply = 0;
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRACKREFRESHSCREEN:
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
      case TRACKSETTRACK:
	activeTrack = msg.data[0];
	reply = 0;
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRACKGETTRACK:
	reply = activeTrack;
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRACKREMOVETASK:
	tempNode = first;
	while(tempNode != NULL) {
	  if(tempNode->tid == msg.tid) {
	    if(tempNode->next == NULL && tempNode->last == NULL) {
	      first = last = NULL;
	    }else if(tempNode->next == NULL) {
	      last = tempNode->last;
	      last->next = NULL;
	    }else if(tempNode->last == NULL) {
	      first = tempNode->next;
	      first->last = NULL;
	    }else{
	      (tempNode->last)->next = tempNode->next;
	      (tempNode->next)->last = tempNode->last;
	    }
	  }
	  tempNode = tempNode->next;
	}
	Reply(src, (char *)&reply, sizeof(int));
	break;
    }
  }
}

void removeSensorTask(int taskID) {
  struct TrainMessage msg;
  msg.type = TRACKREMOVETASK;
  msg.tid = taskID;
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

void printTime(int min, int sec, int tenthSec) {
  struct TrainMessage msg;
  msg.type = TRACKPRINTCLOCK;
  msg.data[0] = min;
  msg.data[1] = sec;
  msg.data[2] = tenthSec;
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

  

void setSwitchState(int switchNum, char state) {
  struct TrainMessage msg;
  msg.type = TRACKSETSWITCH;
  msg.data[0] = switchNum;
  msg.data[1] = (int)state;
  int reply;

  int server = whoIs("Train Server");
  while(server == -1) server = whoIs("Train Server");

  Send(server, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

char getSwitchState(int switchNum) {
  struct TrainMessage msg;
  msg.type = TRACKGETSWITCH;
  msg.data[0] = switchNum;
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));

  return (char)reply;
}

void waitOnSensor(int sensorNum) {
  struct TrainMessage msg;
  msg.type = TRACKBLOCKSENSOR;
  struct SensorStates states;
  setSensor(&states, sensorNum, true);
  msg.data[0] = states.stateInfo[0];
  msg.data[1] = states.stateInfo[1];
  msg.data[2] = states.stateInfo[2];
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

int waitOnSensors(struct SensorStates *sensors) {
  struct TrainMessage msg;
  msg.type = TRACKBLOCKSENSOR;
  msg.data[0] = (sensors->stateInfo)[0];
  msg.data[1] = (sensors->stateInfo)[1];
  msg.data[2] = (sensors->stateInfo)[2];
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));

  return reply;
}

int waitOnAnySensor() {
  struct TrainMessage msg;
  msg.type = TRACKBLOCKSENSOR;
  msg.data[0] = 0xFFFFFFFF;
  msg.data[1] = 0xFFFFFFFF;
  msg.data[2] = 0xFFFFFFFF;
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));

  return reply;
}

void setSensorData(struct SensorStates *states) {
  struct TrainMessage msg;
  msg.type = TRACKSETSENSORS;
  msg.data[0] = states->stateInfo[0];
  msg.data[1] = states->stateInfo[1];
  msg.data[2] = states->stateInfo[2];
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

void printDebugInfo(int line, char *debugInfo) {
  struct TrainMessageBuf msg;
  msg.type = TRACKPRINTDEBUG;
  msg.data[0] = line;
  strcpy(msg.buffer, debugInfo);
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessageBuf), (char *)&reply, sizeof(int));
}

void refreshScreen() {
  struct TrainMessage msg;
  msg.type = TRACKREFRESHSCREEN;
  int reply;

  int server = whoIs("Train Server");
  while(server == -1) {
    server = whoIs("Train Server");
  }

  Send(server, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

void setTrack(int track) {
  struct TrainMessage msg;
  msg.type = TRACKSETTRACK;
  msg.data[0] = track;
  int reply;

  Send(whoIs("Train Server"), (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

int getTrack() {
  struct TrainMessage msg;
  msg.type = TRACKGETTRACK;
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
