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

#define NUMCOMMANDS 	9
#define MAX_ARGS	25

#define TRAINGOTO	0
int moveToLocation(int trainNum, int source, int dest, track_node *track, int doReverse, int speed);
int BFS(int node1, int node2, track_node *track, struct Path *path, int doReverse);
void trainTracker();
void trainTask();
struct TrainMessage {
  int type;
  char dest[8];
  int doReverse;
  int speed;
};

char *splitCommand(char *command);

int largestPrefix(char *str1, char *str2) {
  int i=0;
  while(str1[i] != '\0' && str1[i] == str2[i]) i++;
  return i;
}

int tabComplete(char *command) {
  char *commands[NUMCOMMANDS] = {"q", "tr", "rv", "sw", "setTrack", "move", "randomizeSwitches", "init", "clear"};

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

  if(maxMatched == length) {
    outputEscape("[B[100D");
    for(i=0; i<NUMCOMMANDS; i++) {
      int prefix = largestPrefix(command, commands[i]);
      if(maxMatched == prefix) {
	printf("%s\t", commands[i]);
      }
    }
    outputEscape("[B");
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

int formatArgs(char *command, char **argv) {
  char *lastArg = command;
  char *nextArg = splitCommand(lastArg);
  argv[0] = lastArg;
  int argNum = 1;

  while(nextArg != lastArg) {
    argv[argNum++] = nextArg;
    lastArg = nextArg;
    nextArg = splitCommand(lastArg);
  }
  return argNum;
}

int numArgs(int argc, char *argv[]) {
  int i, numArgs = 0;

  for(i=1; i<argc; i++) {
    if(argv[i][0] != '-') {
      ++numArgs;
    }
  }
  return numArgs;
}

int numFlags(int argc, char *argv[]) {
  return argc - numArgs(argc, argv) - 1;
}

char *getArgument(int argc, char *argv[], int argNum) {
  int i, curArg = 0;

  for(i=1; i<argc; i++) {
    if(argv[i][0] != '-') {
      if(argNum == curArg) return argv[i];
      ++curArg;
    }
  }
  return NULL;
}

int getFlag(int argc, char *argv[], char *flag) {
  int i;

  for(i=1; i<argc; i++) {
    if(argv[i][0] == '-') {
      if(strcmp(argv[i]+1, flag) == 0) {
	return true;
      }
    }
  }
  return false;
}

void parseCommand(char *command, int *trainSpeeds, int *train) {
  char *argv[MAX_ARGS];
  int argc = formatArgs(command, argv);

  if(strcmp(argv[0], "tr") == 0) {
    if(numArgs(argc, argv) != 2) {
      printColored(RED, BLACK, "Error: command tr expects 2 arguments\r");
    }else{
      int trainNumber = strToInt(getArgument(argc, argv, 0));
      int trainSpeed = strToInt(getArgument(argc, argv, 1));
      if(trainNumber == -1 || trainNumber < 1 || trainNumber > 80) {
	printColored(RED, BLACK, "Error: train number must be a number between 1 and 80\r");
      }else if(trainSpeed == -1 || trainSpeed < 0 || trainSpeed > 14) {
	printColored(RED, BLACK, "Error: train speed must be a number between 0 and 14\r");
      }else {
	Putc2(1, (char)trainSpeed, (char)trainNumber);
	trainSpeeds[trainNumber] = trainSpeed;
      }
    }
  }else if(strcmp(argv[0], "rv") == 0) {
    if(numArgs(argc, argv) != 1) {
      printColored(RED, BLACK, "Error: command rv expects an argument\r");
    }else{
      int trainNumber = strToInt(getArgument(argc, argv, 0));
      if(trainNumber == -1 || trainNumber < 1 || trainNumber > 80) {
	printColored(RED, BLACK, "Error: train number must be a number between 1 and 80\r");
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
  }else if(strcmp(argv[0], "sw") == 0) {
    if(numArgs(argc, argv) != 2) {
      printColored(RED, BLACK, "Error: command sw expects two arguments\r");
    }else{
      int switchNumber = strToInt(getArgument(argc, argv, 0));
      if(strcmp(getArgument(argc, argv, 1), "S") == 0) {
        setSwitchState(switchNumber, 'S');
      }else if(strcmp(getArgument(argc, argv, 1), "C") == 0) {
	setSwitchState(switchNumber, 'C');
      }else{
	printColored(RED, BLACK, "Error: switch direction must be 'S' or 'C'\r");
      }
    }
  }else if(strcmp(argv[0], "q") == 0) {
    outputEscape("[2J");
    moveCursor(1,1);
    Shutdown();
  }else if(strcmp(argv[0], "setTrack") == 0) {
    if(numArgs(argc, argv) != 1) {
      printColored(RED, BLACK, "Error: command setTrack expects an argument\r");
    }else{
      if(strcmp(getArgument(argc, argv, 0), "A") == 0) {
	setTrack(TRACKA);
      }else if(strcmp(getArgument(argc, argv, 0), "B") == 0) {
	setTrack(TRACKB);
      }else{
	printColored(RED, BLACK, "Error: track must be A or B\r");
      }
    }
  }else if(strcmp(argv[0], "move") == 0) {
    if(numArgs(argc, argv) != 3) {
      printColored(RED, BLACK, "Error: command move expects two arguments\r");
    }else{
      struct TrainMessage msg;
      int trainNum = strToInt(getArgument(argc, argv, 0));
      msg.type = TRAINGOTO;
      strcpy(msg.dest, getArgument(argc, argv, 1));
      msg.doReverse = getFlag(argc, argv, "r");
      msg.speed = strToInt(getArgument(argc, argv, 2));
      if(train[trainNum] == -1) {
	printColored(RED, BLACK, "Error: train %d has not been initialized\r", trainNum);
      }else{
        int reply;
        Send(train[trainNum], (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
      }
    }
  }else if(strcmp(argv[0], "randomizeSwitches") == 0) {
    int i;
    seed(Time());
    for(i=1; i<19; i++) {
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
  }else if(strcmp(argv[0], "init") == 0) {
    if(numArgs(argc, argv) != 1) {
      printColored(RED, BLACK, "Error: command init expects an argument\r");
    }else{
      int trainNum = strToInt(getArgument(argc, argv, 0));
      if(train[trainNum] == -1) {
	train[trainNum] = Create(2, trainTask);
	int reply;
	Send(train[trainNum], (char *)&trainNum, sizeof(int), (char *)&reply, sizeof(int));
      }else{
	printf("Train %d has already been initialized\r");
      }
    }
  }else if(strcmp(argv[0], "d") == 0) {
    track_node track[TRACK_MAX];
    initTrack(track);
    int trainNum = 45;
    int trainSpeed = strToInt(getArgument(argc, argv, 0));
    Putc2(1, (char)trainSpeed, (char)trainNum);
    int sensor, lastSensor = waitOnAnySensor(), lastTime = Time(), time;
    int v = 0;
    while(true) {
      sensor = waitOnAnySensor();
      time = Time();
      if(sensor == 71) break;

      int timeDelta = (time - lastTime);
      int distance = BFS(lastSensor, sensor, track, NULL, false);
      int newVelocity = (500*distance)/timeDelta;
      v *= 95;
      v += newVelocity;
      v /= 100;
      lastTime = time;
      lastSensor = sensor;
      printAt(9, 1, "Velocity: %dmm/s\r", v);
    }
    Putc2(1, (char)0, (char)trainNum);
  }else if(strcmp(argv[0], "a") == 0) {
    track_node track[TRACK_MAX];
    initTrack(track);
    int trainNum = 45;
    int trainSpeed = strToInt(getArgument(argc, argv, 2));
    int source = 0, dest = 0;
    int velocity[15];
    initVelocities(trainNum, velocity);
    while(source < TRACK_MAX && strcmp(track[source].name, getArgument(argc, argv, 0)) != 0) source++;
    while(dest < TRACK_MAX && strcmp(track[dest].name, getArgument(argc, argv, 1)) != 0) dest++;

    int distance = BFS(source, dest, track, NULL, false);

    Putc2(1, (char)trainSpeed, (char)trainNum);
    int t0 = Time();
    printf("Time 0: %d\r", t0);
    waitOnSensor(dest);
    int t2 = Time();
    Putc2(1, (char)0, (char)trainNum);
    printf("Time 2: %d\r", t2);

    int t1 = -(200*distance)/velocity[trainSpeed];
    t1 -= t0;
    t1 += 2*t2;
    printf("Time 1: %d\r", t1);
    printf("Accelerating to speed %d takes %d ticks\r", trainSpeed, t1-t0);
  }else if(strcmp(argv[0], "clear") == 0) {
    moveCursor(13, 1);
    outputEscape("[J");
  }else{
    printColored(RED, BLACK, "Unrecognized command: \"%s\"\r", command);
  }
}

void copyPath(struct Path *dest, struct Path *source) {
  int i=0;
  dest->numNodes = source->numNodes;
  for(i=0; i < source->numNodes; i++) {
    (dest->node)[i] = (source->node)[i];
  }
}

int BFS(int node1, int node2, track_node *track, struct Path *path, int doReverse) {
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
    if(doReverse && (first->reverse)->discovered == false) {
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
      if(distance > 300) {
        *returnDistance = -300;
      }else{
        *returnDistance = -300 + distance;
      }
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
    base += 10;	//if this changes must also change value in updateProfile
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
  int dest;
  while(true) {
    Receive(&src, (char *)&msg, sizeof(struct TrainMessage));
    switch(msg.type) {
      case TRAINGOTO:
	dest = 0;
	while(dest < TRACK_MAX && strcmp(track[dest].name, msg.dest) != 0) dest++;
	location = moveToLocation(trainNum, location, dest, track, msg.doReverse, msg.speed);
	Reply(src, (char *)&reply, sizeof(int));
	break;
    }
  } 
}

int findNextReverseNode(struct Path *path, int curNode) {
  if(((path->node)[curNode-1])->reverse == (path->node)[curNode]) {
    return -1;
  }

  int offset = 0, node;
  int distance;
  while(curNode < path->numNodes) {
    int reverseDistance = stoppingDistance(700);
    node = distanceBefore(path, reverseDistance, curNode+offset, &distance);
    if(node > curNode) break;

    if(((path->node)[curNode+offset])->reverse == (path->node)[curNode+offset+1]) {
      return curNode+offset;
    }
    offset++;
  }
  return -1;
}

int moveToLocation(int trainNum, int source, int dest, track_node *track, int doReverse, int speed) {
  struct Path path;
  struct VelocityProfile profile;

  BFS(source, dest, track, &path, doReverse);
  if((path.node[path.numNodes-2])->reverse == path.node[path.numNodes-1]) {
    path.numNodes--;
  }

  int periodic = Create(2, periodicTask);

  initProfile(&profile, trainNum, speed, &path, source, periodic);

  int curNode = 1;

  profile.location = curNode-1;

  int timeout = false;
  int switchNode, switchDistance;
  Putc2(1, (char)speed, (char)trainNum);
  setAccelerating(&profile);

  while(curNode < path.numNodes) {
    if((path.node[curNode-1])->reverse == path.node[curNode]) {
      waitForStop(&profile);
      if(curNode != 1) {
        profile.location = curNode;
        profile.delta = -300;
      }
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

    if(curNode == path.numNodes-1) break;

    int distance = adjDistance(path.node[curNode-1], path.node[curNode]);
    if(curNode == 1 && (path.node[0])->reverse == path.node[1]) {
    }else if((path.node[curNode])->type == NODE_SENSOR) {
      int reply;
      int sensorNum = (path.node[curNode])->num;
      int sensorTask = Create(2, sensorWaitTask);
      Send(sensorTask, (char *)&sensorNum, sizeof(int), (char *)&reply, sizeof(int));

      int src = waitForDistance(&profile, distance+150);
      if(src == periodic) {
	printf("timeout at sensor %s\r", (path.node[curNode])->name);
        timeout = true;
      }else{
	float err = profile.delta - distance;
	printf("distance error at node %s: %dmm\r", (path.node[curNode])->name, (int)err);
      }
      Destroy(sensorTask);
    }else{
      waitForDistance(&profile, distance);
    }

    setLocation(&profile, curNode);
    curNode++;
    if(timeout) {
      profile.delta = 150;
      timeout = false;
    }
  }
  int reply;
  int sensorNum = (path.node[curNode])->num;
  int sensorTask = Create(2, sensorWaitTask);
  Send(sensorTask, (char *)&sensorNum, sizeof(int), (char *)&reply, sizeof(int));

  int distance = adjDistance(path.node[curNode-1], path.node[curNode]);
  int src = waitForDistanceOrStop(&profile, distance);
  if(src == periodic) {
    printf("timeout at sensor %s\r", (path.node[curNode])->name);
  }else{
    float err = profile.delta - distance;
    printf("distance error at node %s: %dmm\r", (path.node[curNode])->name, (int)err);
  }
  Destroy(sensorTask);

  setLocation(&profile, path.numNodes-1);
  waitForStop(&profile);
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

  Create(2, clockDriver);

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
	Putc(2, '\r');
	parseCommand(curCommand, trainSpeeds, train);
	commandNum++;
	commandNum %= NUM_SAVED_COMMANDS;
	viewingNum = commandNum;
	commands[commandNum][0] = '\0';
	commandLength = 0;
	if(totalCommands < NUM_SAVED_COMMANDS-1) totalCommands++;
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
