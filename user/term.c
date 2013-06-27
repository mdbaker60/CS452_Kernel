#include <bwio.h>
#include <term.h>
#include <string.h>
#include <syscall.h>
#include <io.h>
#include <values.h>
#include <clockServer.h>
#include <train.h>
#include <track.h>

#define NUMCOMMANDS 6

void trainTracker();
void trainController();
struct TrainMessage {
  int trainNum;
  char dest[8];
};

char *splitCommand(char *command);

int largestPrefix(char *str1, char *str2) {
  int i=0;
  while(str1[i] != '\0' && str1[i] == str2[i]) i++;
  return i;
}

int tabComplete(char *command) {
  char *commands[NUMCOMMANDS] = {"q", "tr", "rv", "sw", "setTrack", "move"};

  int length = strlen(command);
  char *arg1 = splitCommand(command);
  if(arg1 != command) {
    *(arg1-1) = ' ';
    return length;
  }

  int maxMatched = -1;
  char *matchedIn;
  int i;
  for(i=0; i<NUMCOMMANDS; i++) {
    int prefix = largestPrefix(command, commands[i]);
    if(maxMatched == -1 && prefix >= length) {
      maxMatched = strlen(commands[i]);
      matchedIn = commands[i];
    }else if(prefix >= length) {
      maxMatched = largestPrefix(matchedIn, commands[i]);
    }
  }

  if(maxMatched == -1) {
    return length;
  }
  for(i=0; i<maxMatched; i++) {
    command[i] = matchedIn[i];
  }
  command[maxMatched] = '\0';

  return maxMatched;
}

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

  Delay(250);
  //reverse command
  Putc2(1, (char)15, (char)info.number);
  //back to speed command
  Putc2(1, (char)info.speed, (char)info.number);

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
    printTime(mins, secs, tenthSecs);
  }
}

void parseCommand(char *command, int *trainSpeeds) {
  char *arg1 = splitCommand(command);
  char *arg2 = splitCommand(arg1);

  if(strcmp(command, "tr") == 0) {
    if(arg1 == arg2) {
      printColored(RED, BLACK, "\rError: command tr expects 2 arguments");
    }else{
      int trainNumber = strToInt(arg1);
      int trainSpeed = strToInt(arg2);
      if(trainNumber == -1 || trainNumber < 1 || trainNumber > 80) {
	printColored(RED, BLACK, "\rError: train number must be a number between 1 and 80");
      }else if(trainSpeed == -1 || trainSpeed < 0 || trainSpeed > 14) {
	printColored(RED, BLACK, "\rError: train speed must be a number between 0 and 14");
      }else {
	Putc2(1, (char)trainSpeed, (char)trainNumber);
	trainSpeeds[trainNumber] = trainSpeed;
      }
    }
  }else if(strcmp(command, "rv") == 0) {
    if(arg1 == command) {
      printColored(RED, BLACK, "\rError: command rv expects an argument");
    }else{
      int trainNumber = strToInt(arg1);
      if(trainNumber == -1 || trainNumber < 1 || trainNumber > 80) {
	printColored(RED, BLACK, "\rError: train number must be a number between 1 and 80");
      }else{
	Putc2(1, (char)0, (char)trainNumber);
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
      printColored(RED, BLACK, "\rError: command sw expects two arguments");
    }else{
      int switchNumber = strToInt(arg1);
      char switchDirection;
      if(arg2[1] == '\0') {
        switchDirection = arg2[0];
      }else{
	switchDirection = '!';
      }
      if(switchDirection != 'S' && switchDirection != 'C') {
	printColored(RED, BLACK, "\rError: switch direction must be 'S' or 'C'");
      }else if(switchDirection == 'S') {
	Putc2(1, (char)33, (char)switchNumber);
	Putc(1, (char)32);
	setSwitchState(switchNumber, switchDirection);
      }else if(switchDirection == 'C') {
	Putc2(1, (char)34, (char)switchNumber);
	Putc(1, (char)32);
	setSwitchState(switchNumber, switchDirection);
      }
    }
  }else if(strcmp(command, "q") == 0) {
    Shutdown();
  }else if(strcmp(command, "setTrack") == 0) {
    if(arg1 == command) {
      printColored(RED, BLACK, "\rError: command setTrack expects an argument\r");
    }else{
      if(arg1[1] == '\0') {
	if(arg1[0] == 'A') {
	  setTrack(TRACKA);
	}else if(arg1[0] == 'B') {
	  setTrack(TRACKB);
	}else {
	  printColored(RED, BLACK, "\rError: track must be A or B\r");
	}
      }else{
	printColored(RED, BLACK, "\rError: track must be A or B\r");
      }
    }
  }else if(strcmp(command, "move") == 0) {
    if(arg1 == arg2) {
      printColored(RED, BLACK, "\rError: command move expects two arguments\r");
    }else{
      struct TrainMessage msg;
      msg.trainNum = strToInt(arg1);
      strcpy(msg.dest, arg2);
      int reply, tid = Create(2, trainController);
      Send(tid, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
    }
  }else{
    printColored(RED, BLACK, "\rUnrecognized command: \"%s\"", command);
  }
  printf("\r>");
}

void copyPath(struct Path *dest, struct Path *source) {
  int i=0;
  dest->numEdges = source->numEdges;
  for(i=0; i < source->numEdges; i++) {
    (dest->edge)[i] = (source->edge)[i];
  }
}

int BFS(int node1, int node2, track_node *track, struct Path *path) {
  track_node *start = &track[node1];
  track_node *stop = &track[node2];

  track_node *first = start;
  track_node *last = start;

  int i;
  for(i=0; i<TRACK_MAX; i++) {
    track[i].discovered = false;
    track[i].next = NULL;
  }
  first->discovered = true;
  first->searchDistance = 0;
  (first->path).numEdges = 0;

  while(first != NULL) {
    if(first == stop) {
      if(path != NULL) {
	copyPath(path, &(first->path));
      }
      return first->searchDistance;
    }
    if(first->type == NODE_SENSOR || first->type == NODE_MERGE || first->type == NODE_ENTER || first->type == NODE_EXIT) {
      if(((first->edge)[0].dest)->discovered == false) {
	last->next = (first->edge)[0].dest;
	last = last->next;
	last->discovered = true;
	last->next = NULL;
	last->searchDistance = first->searchDistance + (first->edge)[0].dist;
	copyPath(&(last->path), &(first->path));
	(last->path).edge[(last->path).numEdges++] = &((first->edge)[0]);
      }
    }else if(first->type == NODE_BRANCH) {
      for(i=0; i<2; i++) {
	if(((first->edge)[i].dest)->discovered == false) {
	  last->next = (first->edge)[i].dest;
	  last = last->next;
	  last->discovered = true;
	  last->next = NULL;
	  last->searchDistance = first->searchDistance + (first->edge)[i].dist;
	  copyPath(&(last->path), &(first->path));
	  (last->path).edge[(last->path).numEdges++] = &(first->edge)[i];
	}
      }
    }
    first = first->next;
  }

  return 0;
}

void trainController() {
  int v = 225;
  track_node track[TRACK_MAX];
  initTrack(track);

  int src, reply = 0;
  struct TrainMessage msg;
  Receive(&src, (char *)&msg, sizeof(struct TrainMessage));
  Reply(src, (char *)&reply, sizeof(int));

  int dest = 0, trainNum = msg.trainNum;
  while(dest < TRACK_MAX && strcmp(track[dest].name, msg.dest) != 0) dest++;

  struct Path path;
  Putc2(1, (char)2, (char)trainNum);
  int source = waitOnAnySensor();
  Putc2(1, (char)0, (char)trainNum);
  Putc2(1, (char)10, (char)trainNum);

  BFS(source, dest, track, &path);
  /*int i;
  for(i=0; i<path.numEdges; i++) {
    printf("%s ->", ((path.edge[i])->src)->name);
  }
  printf("%s\r", ((path.edge[i-1])->dest)->name);*/

  int curEdge = 0;
  node_type nodeType;
  while(true) {
    nodeType = ((path.edge[curEdge])->dest)->type;
    if(nodeType == NODE_SENSOR) {
      int sensorNum = ((path.edge[curEdge])->dest)->num;
      printf("waiting to hit sensor %s\r", ((path.edge[curEdge])->dest)->name);
      if(path.numEdges > curEdge+1 && ((path.edge[curEdge+1])->dest)->type == NODE_BRANCH) {
	curEdge++;
	if(&(((path.edge[curEdge])->dest)->edge)[DIR_STRAIGHT] == path.edge[curEdge+1]) {
	  Putc2(1, (char)33, (char)((path.edge[curEdge])->dest)->num);
	  Putc(1, (char)32);
	}else{
	  Putc2(1, (char)34, (char)((path.edge[curEdge])->dest)->num);
	  Putc(1, (char)32);
	}
	waitOnSensor(((path.edge[curEdge-1])->dest)->num);
      }else{
        waitOnSensor(((path.edge[curEdge])->dest)->num);
      }
    }else if(nodeType == NODE_BRANCH) {
      if(&((((path.edge[curEdge])->dest)->edge)[DIR_STRAIGHT]) == path.edge[curEdge+1]) {
	Putc2(1, (char)33, (char)((path.edge[curEdge])->dest)->num);
	Putc(1, (char)32);
      }else{
	Putc2(1, (char)34, (char)((path.edge[curEdge])->dest)->num);
	Putc(1, (char)32);
      }
    }

    if((path.edge[curEdge])->dest == &track[dest]) {
      break;
    }
    curEdge++;
  }
  

  Putc2(1, (char)0, (char)trainNum);
  printf("reached destination\r");
  Destroy(MyTid());

  int seconds, time, oldTime = Time(), distance, sensor, oldSensor = waitOnAnySensor();
  while(true) {
    sensor = waitOnAnySensor();
    time = Time();
    distance = BFS(oldSensor, sensor, track, NULL);
    seconds = (time - oldTime);
    v *= 90;
    v += (500*distance)/seconds;
    v /= 100;
    oldTime = time;
    oldSensor = sensor;
  }
}

void trainTracker() {
  int v = 425;
  track_node track[TRACK_MAX];
  initTrack(track);

  int src, trainNum, reply = 0;
  Receive(&src, (char *)&trainNum, sizeof(int));
  Reply(src, (char *)&reply, sizeof(int));

  char debugMessage[19];
  strcpy(debugMessage, "Velocity: 000 mm/s");

  int seconds, time, oldTime = Time(), distance, sensor, oldSensor = waitOnAnySensor();
  while(true) {
    sensor = waitOnAnySensor();
    time = Time();
    distance = BFS(oldSensor, sensor, track, NULL);
    seconds = (time - oldTime);
    v *= 90;
    v += (500*distance)/seconds;
    v /= 100;
    debugMessage[10] = (char)((v/100)%10)+48;
    debugMessage[11] = (char)((v/10)%10)+48;
    debugMessage[12] = (char)(v%10)+48;
    printDebugInfo(1, debugMessage);

    if(sensor == 71) {
      Putc2(1, (char)0, (char)trainNum);
    }

    oldTime = time;
    oldSensor = sensor;
  }
}

void terminalDriver() {
  char commands[NUM_SAVED_COMMANDS][BUFFERSIZE];
  int lengths[NUM_SAVED_COMMANDS];
  int commandNum = 0, viewingNum = 0;

  char curCommand[BUFFERSIZE];
  int commandLength = 0, totalCommands = 0;

  char c;

  int trainSpeeds[80];
  int i;
  for(i=0; i<80; i++) {
    trainSpeeds[i] = 0;
  }

  Create(1, clockDriver);

  outputEscape("[2J");
  moveCursor(13, 1);
  Putc(2, '>');
  refreshScreen();
  while(true) {
    c = (char)Getc(2);
    switch(c) {
      case '\r':
	curCommand[commandLength] = '\0';
	strcpy(commands[commandNum], curCommand);
	lengths[commandNum] = commandLength;
	parseCommand(curCommand, trainSpeeds);
	commandNum++;
	commandNum %= NUM_SAVED_COMMANDS;
	viewingNum = commandNum;
	commands[commandNum][0] = '\0';
	commandLength = 0;
	if(totalCommands < NUM_SAVED_COMMANDS-1) totalCommands++;
	refreshScreen();
	break;
      case '\t':
	if(commandLength > 0) {
	  curCommand[commandLength] = '\0';
	  commandLength = tabComplete(curCommand);
	  outputEscape("[100D[K");
	  printf(">%s", curCommand);
	}
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
	  if(commandNum == viewingNum) {
	    curCommand[commandLength] = '\0';
	    strcpy(commands[commandNum], curCommand);
	    lengths[commandNum] = commandLength;
	  }
	  if(viewingNum != ((commandNum + NUM_SAVED_COMMANDS - totalCommands) 
					% NUM_SAVED_COMMANDS)) {
	    viewingNum += NUM_SAVED_COMMANDS - 1;
	    viewingNum %= NUM_SAVED_COMMANDS;
	    strcpy(curCommand, commands[viewingNum]);
	    commandLength = lengths[viewingNum];
	    outputEscape("[100D[K");
	    printf(">%s", curCommand);
	  }
	}else if(c == 'B') {
	  if(viewingNum != commandNum) {
	    viewingNum += 1;
	    viewingNum %= NUM_SAVED_COMMANDS;
	    strcpy(curCommand, commands[viewingNum]);
	    commandLength = lengths[viewingNum];
	    outputEscape("[100D[K");
	    printf(">%s", curCommand);
	  }
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
