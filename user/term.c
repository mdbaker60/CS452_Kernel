#include <bwio.h>
#include <term.h>
#include <string.h>
#include <syscall.h>
#include <io.h>
#include <values.h>
#include <clockServer.h>
#include <train.h>
#include <track.h>
#include <prng.h>

#define NUMCOMMANDS 8

#define TRAINGOTO	0
void moveToLocation(int trainNum, int source, int dest, int *velocity, track_node *track);
void trainTracker();
void trainTask();
struct TrainMessage {
  int type;
  char dest[8];
};

char *splitCommand(char *command);

int largestPrefix(char *str1, char *str2) {
  int i=0;
  while(str1[i] != '\0' && str1[i] == str2[i]) i++;
  return i;
}

int tabComplete(char *command) {
  char *commands[NUMCOMMANDS] = {"q", "tr", "rv", "sw", "setTrack", "move", "randomizeSwitches", "init"};

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
  if(maxMatched == strlen(matchedIn)) {
    command[maxMatched] = ' ';
    maxMatched++;
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

void initializeTrack() {
  char trackDirection[22] = {'S', 'S', 'C', 'S', 'C', 'S', 'S', 'C', 'C', 'C', 'C', 'C', 'C', 'C', 'C', 'C', 'C', 'C', 'C', 'C', 'C', 'C'};

  Delay(10);
  int i;
  for(i=1; i<19; i++) {
    setSwitchState(i, trackDirection[i-1]);
  }
  for(i=153; i<157; i++) {
    setSwitchState(i, trackDirection[i-135]);
  }
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

void parseCommand(char *command, int *trainSpeeds, int *train) {
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
      }else{
	setSwitchState(switchNumber, switchDirection);
      }
    }
  }else if(strcmp(command, "q") == 0) {
    Shutdown();
  }else if(strcmp(command, "setTrack") == 0) {
    if(arg1 == command) {
      printColored(RED, BLACK, "\rError: command setTrack expects an argument");
    }else{
      if(arg1[1] == '\0') {
	if(arg1[0] == 'A') {
	  setTrack(TRACKA);
	}else if(arg1[0] == 'B') {
	  setTrack(TRACKB);
	}else {
	  printColored(RED, BLACK, "\rError: track must be A or B");
	}
      }else{
	printColored(RED, BLACK, "\rError: track must be A or B");
      }
    }
  }else if(strcmp(command, "move") == 0) {
    if(arg1 == arg2) {
      printColored(RED, BLACK, "\rError: command move expects two arguments");
    }else{
      struct TrainMessage msg;
      int trainNum = strToInt(arg1);
      msg.type = TRAINGOTO;
      strcpy(msg.dest, arg2);
      if(train[trainNum] == -1) {
	printColored(RED, BLACK, "\rError: train %d has not been initialized", trainNum);
      }else{
        int reply;
        Send(train[trainNum], (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
      }
    }
  }else if(strcmp(command, "randomizeSwitches") == 0) {
    int i;
    seed(Time());
    for(i=0; i<18; i++) {
      int dir = random() % 2;
      if(dir == 0) {
	setSwitchState(i, 'S');
      }else{
	setSwitchState(i, 'C');
      }
    }
    for(i=153; i<157; i++) {
      int dir = random() % 2;
      if(dir == 0) {
	setSwitchState(i, 'S');
      }else{
	setSwitchState(i, 'C');
      }
    }
  }else if(strcmp(command, "init") == 0) {
    if(command == arg1) {
      printColored(RED, BLACK, "\rError: command init expects an argument");
    }else{
      int trainNum = strToInt(arg1);
      if(train[trainNum] == -1) {
	train[trainNum] = Create(1, trainTask);
	int reply;
	Send(train[trainNum], (char *)&trainNum, sizeof(int), (char *)&reply, sizeof(int));
      }else{
	printf("\rTrain %d has already been initialized");
      }
    }
  }else{
    printColored(RED, BLACK, "\rUnrecognized command: \"%s\"", command);
  }
}

void copyPath(struct Path *dest, struct Path *source) {
  int i=0;
  dest->numNodes = source->numNodes;
  for(i=0; i < source->numNodes; i++) {
    (dest->node)[i] = (source->node)[i];
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
  (first->path).numNodes = 1;
  (first->path).node[0] = start;

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
	(last->path).node[(last->path).numNodes++] = (first->edge)[0].dest;
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
	  (last->path).node[(last->path).numNodes++] = (first->edge)[i].dest;
	}
      }
    }
    first = first->next;
  }

  return 0;
}

void getVelocities(int trainNum, int *velocity) {
  if(trainNum == 45) {
    velocity[8] = 376;
    velocity[9] = 419;
    velocity[10] = 446;
    velocity[11] = 505;
    velocity[12] = 552;
    velocity[13] = 600;
    velocity[14] = 601;
  }
}

int stoppingDistance(int velocity) {
  int d = (14195*velocity) - 1020000;
  d /= 10000;
  return d;
}

int adjDistance(track_node *src, track_node *dest) {
  if((src->edge)[0].dest == dest) {
    return (src->edge)[0].dist;
  }else if((src->edge)[1].dest == dest) {
    return (src->edge)[1].dist;
  }
  return 0;
}

int adjDirection(track_node *src, track_node *dest) {
  if((src->edge)[0].dest == dest) {
    return 0;
  }else if((src->edge)[1].dest == dest) {
    return 1;
  }
  return -1;
}

int distanceBefore(struct Path *path, int distance, int nodeNum, int *returnDistance) {
  int diff;
  while(true) {
    if(nodeNum == 0) {
      *returnDistance = 0;
      return -1;
    }else{
      diff = adjDistance((path->node)[nodeNum-1], (path->node)[nodeNum]);
      if(diff < distance) {
	distance -= diff;
	nodeNum--;
      }else{
	*returnDistance = diff - distance;
	return nodeNum-1;
      }
    }
  }

  /*while(true) {
    if(((path->edge)[edgeNum])->dist < distance) {
      distance -= ((path->edge)[edgeNum])->dist;
      edgeNum--;
    }else if(edgeNum >= 0) {
      *returnDistance = ((path->edge)[edgeNum])->dist - distance;
      return ((path->edge)[edgeNum])->src;
    }else{
      *returnDistance = 0;
      return ((path->edge)[0])->src;
    }
  }*/
}

void delayTask() {
  int delayTicks, reply = 0, src;
  Receive(&src, (char *)&delayTicks, sizeof(int));
  Reply(src, (char *)&reply, sizeof(int));

  DelayUntil(delayTicks);

  Send(src, (char *)&delayTicks, sizeof(int), (char *)&reply, sizeof(int));
  Destroy(MyTid());
}

void sensorWaitTask() {
  int sensorNum, reply = 0, src;
  Receive(&src, (char *)&sensorNum, sizeof(int));
  Reply(src, (char *)&reply, sizeof(int));

  waitOnSensor(sensorNum);

  Send(src, (char *)&sensorNum, sizeof(int), (char *)&reply, sizeof(int));
  Destroy(MyTid());
}

void trainTask() {
  int trainNum, reply = 0, src;
  Receive(&src, (char *)&trainNum, sizeof(int));
  Reply(src, (char *)&reply, sizeof(int));

  int velocity[15];
  getVelocities(trainNum, velocity);
  track_node track[TRACK_MAX];
  initTrack(track);

  //find your location
  int location, delta = 0;
  Putc2(1, (char)2, (char)trainNum);
  location = waitOnAnySensor();
  Putc2(1, (char)0, (char)trainNum);

  struct TrainMessage msg;
  while(true) {
    Receive(&src, (char *)&msg, sizeof(struct TrainMessage));
    switch(msg.type) {
      case TRAINGOTO:
	Reply(src, (char *)&reply, sizeof(int));
	int dest = 0;
	while(dest < TRACK_MAX && strcmp(track[dest].name, msg.dest) != 0) dest++;
	moveToLocation(trainNum, location, dest, velocity, track);
	location = dest;
	break;
    }
  } 
}

void moveToLocation(int trainNum, int source, int dest, int *velocity, track_node *track) {
  struct Path path;

  BFS(source, dest, track, &path);


  int stopDistance;
  int stopNode = 
	distanceBefore(&path, stoppingDistance(velocity[10]), path.numNodes-1, &stopDistance);

  int curNode = 1;
  int timeBase = Time();

  if(path.numNodes > 0 && (path.node[1])->type == NODE_BRANCH) {
    if(adjDirection(path.node[1], path.node[2]) == DIR_STRAIGHT) {
      setSwitchState((path.node[1])->num, 'S');
    }else{
      setSwitchState((path.node[1])->num, 'C');
    }
  }

  int switchNode, switchDistance, stopDelay = -1;
  Putc2(1, (char)10, (char)trainNum);
  while(curNode < path.numNodes) {
    if(curNode-1 == stopNode) {
      int delayTime = stopDistance/velocity[10];
      delayTime *= 100;
      delayTime += timeBase;
      stopDelay = Create(2, delayTask);
      int reply;
      Send(stopDelay, (char *)&delayTime, sizeof(int), (char *)&reply, sizeof(int));
    }

    int offset = 0;
    while(true) {
      switchNode = distanceBefore(&path, 200, curNode+offset, &switchDistance);
      if(curNode + offset == path.numNodes) {
	break;
      }else if((path.node[curNode+offset])->type == NODE_BRANCH && switchNode == curNode-1) {
	int delayTime = switchDistance/velocity[10];
	delayTime *= 100;
	delayTime += timeBase;
	int switchWaitTask = Create(2, delayTask);
	int reply, src;
	Send(switchWaitTask, (char *)&delayTime, sizeof(int), (char *)&reply, sizeof(int));
	Receive(&src, (char *)&reply, sizeof(int));
	if(src == stopDelay) {
	  Putc2(1, (char)0, (char)trainNum);
	  Destroy(stopDelay);
	  stopDelay = -1;
	  Receive(&src, (char *)&reply, sizeof(int));
	}
	Destroy(switchWaitTask);
	if(adjDirection(path.node[curNode+offset], path.node[curNode+offset+1]) == DIR_STRAIGHT) {
	  setSwitchState((path.node[curNode+offset])->num, 'S');
	}else{
	  setSwitchState((path.node[curNode+offset])->num, 'C');
	}
      }else if(switchNode > curNode-1) {
	break;
      }
      offset++;
    }
    if(stopDelay != -1) {
      int reply, src;
      Receive(&src, (char *)&reply, sizeof(int));
      Putc2(1, (char)0, (char)trainNum);
      Destroy(stopDelay);
      stopDelay = -1;
    }

    //avoid deadlock if you don't quite hit the final sensor
    if(curNode == path.numNodes-1 && stopDelay == -1) break;

    if((path.node[curNode])->type == NODE_SENSOR) {
      waitOnSensor((path.node[curNode])->num);
      timeBase = Time();
    }else{
      int distance = adjDistance(path.node[curNode-1], path.node[curNode]);
      int delayTime = (100*distance)/velocity[10];
      delayTime += timeBase;
      DelayUntil(delayTime);
      timeBase = Time();
    }

    curNode++;
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
    v *= 95;
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
  int train[80];
  for(i=0; i<80; i++) {
    train[i] = -1;
  }

  Create(1, clockDriver);

  outputEscape("[2J");
  refreshScreen();
  initializeTrack();
  moveCursor(13, 1);
  Putc(2, '>');
  while(true) {
    c = (char)Getc(2);
    switch(c) {
      case '\r':
	curCommand[commandLength] = '\0';
	strcpy(commands[commandNum], curCommand);
	lengths[commandNum] = commandLength;
	parseCommand(curCommand, trainSpeeds, train);
	commandNum++;
	commandNum %= NUM_SAVED_COMMANDS;
	viewingNum = commandNum;
	commands[commandNum][0] = '\0';
	commandLength = 0;
	if(totalCommands < NUM_SAVED_COMMANDS-1) totalCommands++;
	moveCursor(13, 1);
	outputEscape("[K");
	Putc(2, '>');
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
