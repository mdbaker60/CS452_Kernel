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
#include <velocity.h>

#define NUMCOMMANDS 8

#define TRAINGOTO	0
int moveToLocation(int trainNum, int source, int dest, track_node *track);
int BFS(int node1, int node2, track_node *track, struct Path *path);
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
  char *matchedIn = "";
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
    outputEscape("[2J");
    moveCursor(1,1);
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
    for(i=i; i<19; i++) {
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
	train[trainNum] = Create(2, trainTask);
	int reply;
	Send(train[trainNum], (char *)&trainNum, sizeof(int), (char *)&reply, sizeof(int));
      }else{
	printf("\rTrain %d has already been initialized");
      }
    }
  }else if(strcmp(command, "d") == 0) {
    track_node track[TRACK_MAX];
    initTrack(track);
    int trainNum = 45;
    int trainSpeed = strToInt(arg1);
    Putc2(1, (char)trainSpeed, (char)trainNum);
    int sensor, lastSensor = waitOnAnySensor(), lastTime = Time(), time;
    int v = 0;
    while(true) {
      sensor = waitOnAnySensor();
      time = Time();
      if(sensor == 71) break;

      int timeDelta = (time - lastTime);
      int distance = BFS(lastSensor, sensor, track, NULL);
      int newVelocity = (500*distance)/timeDelta;
      v *= 95;
      v += newVelocity;
      v /= 100;
      lastTime = time;
      lastSensor = sensor;
      printAt(9, 1, "Velocity: %dmm/s\r", v);
    }
    Putc2(1, (char)0, (char)trainNum);
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
    if((first->reverse)->discovered == false) {
      last->next = first->reverse;
      last = last->next;
      last->discovered = true;
      last->next = NULL;
      last->searchDistance = first->searchDistance;
      copyPath(&(last->path), &(first->path));
      (last->path).node[(last->path).numNodes++] = first->reverse;
    }
    first = first->next;
  }

  return 0;
}

int adjDistance(track_node *src, track_node *dest) {
  if((src->edge)[0].dest == dest) {
    return (src->edge)[0].dist;
  }else if((src->edge)[1].dest == dest) {
    return (src->edge)[1].dist;
  }else if(src->reverse == dest) {
    return 0;
  }
  return 0;
}

int adjDirection(track_node *src, track_node *dest) {
  if((src->edge)[0].dest == dest) {
    return 0;
  }else if((src->edge)[1].dest == dest) {
    return 1;
  }else if(src->reverse == dest) {
    return 2;
  }
  return -1;
}

int distanceBefore(struct Path *path, int distance, int nodeNum, int *returnDistance) {
  int diff;
  while(true) {
    if(nodeNum == 0) {
      *returnDistance = 0;
      return 0;
    }else if(((path->node)[nodeNum-1])->reverse == (path->node)[nodeNum]) {
      *returnDistance = -1;
      return nodeNum-1;
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

void periodicTask() {
  int parent = MyParentTid();
  int message = 0, reply;
  int base = Time();
  while(true) {
    base += 5;
    DelayUntil(base+10);
    Send(parent, (char *)&message, sizeof(int), (char *)&reply, sizeof(int));
  }
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

  track_node track[TRACK_MAX];
  initTrack(track);

  //find your location
  int location;
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
	location = moveToLocation(trainNum, location, dest, track);
	break;
    }
  } 
}

int findNextReverseNode(struct Path *path, int curNode) {
  if(((path->node)[curNode-1])->reverse == (path->node)[curNode]) {
    return -1;
  }

  int offset = 0, distance, node;
  while(curNode < path->numNodes) {
    node = distanceBefore(path, stoppingDistance(700), curNode+offset, &distance);
    if(node > curNode) break;

    if(((path->node)[curNode+offset])->reverse == (path->node)[curNode+offset+1]) {
      return curNode+offset;
    }
    offset++;
  }
  return -1;
}

int moveToLocation(int trainNum, int source, int dest, track_node *track) {
  struct Path path;
  struct VelocityProfile profile;

  BFS(source, dest, track, &path);
  if((path.node[path.numNodes-2])->reverse == path.node[path.numNodes-1]) {
    path.numNodes--;
  }

  int periodic = Create(2, periodicTask);
  int speed = 14;

  initProfile(&profile, trainNum, speed, &path, source, periodic);


  int curNode = 1;

  if((path.node[0])->reverse == path.node[1]) {
    curNode++;
    Putc2(1, (char)15, (char)trainNum);
  }
  profile.location = curNode-1;

  int switchNode, switchDistance;
  Putc2(1, (char)speed, (char)trainNum);
  setAccelerating(&profile);

  while(curNode < path.numNodes) {
    if((path.node[curNode-1])->reverse == path.node[curNode]) {
      waitForStop(&profile);
      printf("starting 400mm behind node %s\r", (path.node[curNode])->name);
      profile.location = curNode;
      profile.delta = -400;
      Putc2(1, (char)15, (char)trainNum);
      Putc2(1, (char)speed, (char)trainNum);
      setAccelerating(&profile);
    }

    profile.reverseNode = findNextReverseNode(&path, curNode);
    int offset = 0;
    while(curNode+offset < path.numNodes) {
      switchNode = distanceBefore(&path, 200, curNode+offset, &switchDistance);
      if((path.node[curNode+offset])->type == NODE_BRANCH && switchNode == curNode-1) {
	if(switchDistance >= 0) {
          waitForDistance(&profile, switchDistance);
	}
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

    int distance = adjDistance(path.node[curNode-1], path.node[curNode]);
    if((path.node[curNode])->type == NODE_SENSOR) {
      int reply;
      int sensorNum = (path.node[curNode])->num;
      int sensorTask = Create(2, sensorWaitTask);
      Send(sensorTask, (char *)&sensorNum, sizeof(int), (char *)&reply, sizeof(int));

      int src = waitForDistance(&profile, distance+50);
      if(src == periodic) {
	printf("timeout at sensor %s\r", (path.node[curNode])->name);
      }
      Destroy(sensorTask);
    }else{
      waitForDistance(&profile, distance);
    }

    setLocation(&profile, curNode);
    curNode++;
  }
  Destroy(periodic);

  int curLocation = 0;
  while(curLocation < TRACK_MAX && strcmp(track[curLocation].name, 
					  (path.node[path.numNodes-1])->name) != 0) curLocation++;

  return curLocation;
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

  //Create(1, clockDriver);

  outputEscape("[2J[13;100r");
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
        Putc(2, '\r');
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
