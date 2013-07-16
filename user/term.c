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
#include <math.h>

#define NUMCOMMANDS 	10
#define MAX_ARGS	25

#define TRAINUPDATELOCATION	0
#define TRAINGOTO		1
#define TRAINCONFIGVELOCITY	2
#define TRAININIT		3
#define TRAINSETLOCATION	4
#define TRAINSETDISPLAYLOC	5
#define TRAINWAITFORLOCATION	6
#define TRAINWAITFORSTOP	7
#define TRAINSETACC		8
#define TRAINSETDEC		9
#define TRAINGETLOCATION	10
#define TRAINGETVELOCITY	11
#define TRAINGETMAXVELOCITY	12
#define TRAINREMOVETASK		13
#define TRAINGETACCSTATE	14
#define TRAINREVERSE		15
#define TRAINGETDIR		16

#define CHECKSTOP		0
#define SWITCHDONE		1
#define NODEDONE		2
#define SENSORDONE		3
#define SENSORMISSED		4
#define SWITCH0MISSED		5
#define SWITCH1MISSED		6
#define SWITCH2MISSED		7

#define PERIODICSTOP		2

#define TIMEOUTLENGTH		100000
#define REVERSEOVERSHOOT	100000
#define PICKUPLENGTH		48000

typedef enum {
  DIR_FORWARD,
  DIR_BACKWARD
} train_direction;

struct TrainDriverMessage {
  int trainNum;
  int source;
  int dest;
  int doReverse;
  int velocity[15];
};

void waitForLocation(int trainTid, int location, int delta);
void waitForStop(int trainTid);
void moveToLocation(int trainNum, int source, int dest, track_node *track, int doReverse, int *velocity);
int BFS(int node1, int node2, track_node *track, struct Path *path, int doReverse);
void trainTracker();
void trainTask();
void trainDriver();
struct TrainMessage {
  int type;
  char dest[8];
  int doReverse;
  int speed;
  int location;
  int delta;
  int tid;
};

char *splitCommand(char *command);

int largestPrefix(char *str1, char *str2) {
  int i=0;
  while(str1[i] != '\0' && str1[i] == str2[i]) i++;
  return i;
}

int tabComplete(char *command) {
  char *commands[NUMCOMMANDS] = {"q", "tr", "rv", "sw", "setTrack", "move", "randomizeSwitches", "init", "clear", "configureVelocities"};

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
      struct TrainMessage msg;
      int trainNum = strToInt(getArgument(argc, argv, 0));
      int reply;
      if(train[trainNum] == -1) {
	train[trainNum] = Create(2, trainTask);
	Send(train[trainNum], (char *)&trainNum, sizeof(int), (char *)&reply, sizeof(int));
      }
      msg.type = TRAININIT;
      Send(train[trainNum], (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
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
  }else if(strcmp(argv[0], "configureVelocities") == 0) {
    struct TrainMessage msg;
    msg.type = TRAINCONFIGVELOCITY;
    if(argc != 2) {
      printColored(RED, BLACK, "Command configureVelocities expects an argument\r");
    }else{
      int trainNum = strToInt(getArgument(argc, argv, 0));
      int reply;
      if(train[trainNum] == -1) {
	train[trainNum] = Create(2, trainTask);
	Send(train[trainNum], (char *)&trainNum, sizeof(int), (char *)&reply, sizeof(int));
      }
      Send(train[trainNum], (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
    }
  }else{
    printColored(RED, BLACK, "Unrecognized command: \"%s\"\r", command);
  }
}

void addNodeToFrontOfPath(struct Path *path, track_node *newNode) {
  int i;
  for(i = path->numNodes; i>0; --i) {
    (path->node)[i] = (path->node[i-1]);
  }
  (path->node)[0] = newNode;
  (path->numNodes)++;
}

void copyPath(struct Path *dest, struct Path *source) {
  int i=0;
  dest->numNodes = source->numNodes;
  for(i=0; i < source->numNodes; i++) {
    (dest->node)[i] = (source->node)[i];
  }
}

int shortestPath(int node1, int node2, track_node *track, struct Path *path, int doReverse) {
  track_node *start = &track[node1];
  track_node *last = &track[node2];

  int i;
  for(i=0; i<TRACK_MAX; i++) {
    track[i].discovered = false;
    track[i].searchDistance = 0x7FFFFFFF;
  }
  start->searchDistance = 0;
  (start->path).numNodes = 1;
  (start->path).node[0] = start;

  int minDist, altDist;
  track_node *minNode = NULL, *neighbour;
  while(true) {
    minDist = 0x7FFFFFFF;
    for(i=0; i<TRACK_MAX; i++) {
      if(!(track[i].discovered) && minDist > track[i].searchDistance) {
	minDist = track[i].searchDistance;
	minNode = &track[i];
      }
    }

    if(minDist == 0x7FFFFFFF) break;
    minNode->discovered = true;
    switch(minNode->type) {
      case NODE_BRANCH:
	neighbour = ((minNode->edge)[DIR_CURVED]).dest;
        altDist = minNode->searchDistance + adjDistance(minNode, neighbour);
        if(!(neighbour->discovered) && altDist < neighbour->searchDistance) {
	  neighbour->searchDistance = altDist;
	  copyPath(&(neighbour->path), &(minNode->path));
	  (neighbour->path).node[(neighbour->path).numNodes++] = neighbour;
        }
      case NODE_SENSOR:
      case NODE_MERGE:
      case NODE_ENTER:
      case NODE_EXIT:
        neighbour = ((minNode->edge)[DIR_AHEAD]).dest;
        altDist = minNode->searchDistance + adjDistance(minNode, neighbour);
        if(!(neighbour->discovered) && altDist < neighbour->searchDistance) {
	  neighbour->searchDistance = altDist;
	  copyPath(&(neighbour->path), &(minNode->path));
	  (neighbour->path).node[(neighbour->path).numNodes++] = neighbour;
        }
	break;
    }
    neighbour = minNode->reverse;
    altDist = minNode->searchDistance + 1000000;
    if(doReverse && !(neighbour->discovered) && altDist < neighbour->searchDistance) {
      neighbour->searchDistance = altDist;
      copyPath(&(neighbour->path), &(minNode->path));
      (neighbour->path).node[(neighbour->path).numNodes++] = neighbour;
    }
  }

  copyPath(path, &(last->path));
  return last->searchDistance;
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
    return 1000*(src->edge)[0].dist;
  }else if((src->edge)[1].dest == dest) {
    return 1000*(src->edge)[1].dist;
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

int distanceAfter(struct Path *path, int distance, int nodeNum, int *returnDistance) {
  int diff;

  if(distance < 0) {
    return distanceBefore(path, -distance, nodeNum, returnDistance);
  }

  while(true) {
    if(nodeNum == path->numNodes-1) {
      *returnDistance = 0;
      return path->numNodes-1;
    }else if(((path->node)[nodeNum])->reverse == (path->node)[nodeNum+1]) {
      *returnDistance = distance;
      return nodeNum;
    }else{
      diff = adjDistance((path->node)[nodeNum], (path->node)[nodeNum+1]);
      if(diff < distance) {
	distance -= diff;
	nodeNum++;
      }else{
	*returnDistance = distance;
        return nodeNum;
      }
    }
  }
}

int distanceBefore(struct Path *path, int distance, int nodeNum, int *returnDistance) {
  int diff;

  if(distance < 0) {
    return distanceAfter(path, -distance, nodeNum, returnDistance);
  }

  while(true) {
    if(nodeNum == 0) {
      *returnDistance = 0;
      return 0;
    }else if(((path->node)[nodeNum-1])->reverse == (path->node)[nodeNum]) {
      *returnDistance = -distance;
      return nodeNum;
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

void periodicTask() {
  int parent = MyParentTid();
  int message = 0, reply = 0;
  int base = Time();
  while(reply != PERIODICSTOP) {
    base += 2;	//if this changes must also change value in updateProfile
    DelayUntil(base);
    Send(parent, (char *)&message, sizeof(int), (char *)&reply, sizeof(int));
  }
  Destroy(MyTid());
}

void delayTask() {
  int delayTicks, reply = 0, src;
  Receive(&src, (char *)&delayTicks, sizeof(int));
  Reply(src, (char *)&reply, sizeof(int));

  Delay(delayTicks);

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

void stopWaitTask() {
  int trainTid, reply = 0, src;
  Receive(&src, (char *)&trainTid, sizeof(int));
  Reply(src, (char *)&reply, sizeof(int));

  waitForStop(trainTid);

  Send(src, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));
  Destroy(MyTid());
}

struct LocationInfo {
  int location;
  int delta;
  int trainTid;
};

void locationWaitTask() {
  struct LocationInfo info;
  int reply = 0, src;
  Receive(&src, (char *)&info, sizeof(struct LocationInfo));
  Reply(src, (char *)&reply, sizeof(int));

  waitForLocation(info.trainTid, info.location, info.delta);

  Send(src, (char *)&reply, sizeof(int), (char *)&reply, sizeof(int));
  Destroy(MyTid());
}

struct TrainNode {
  int src;
  int location;
  int delta;
  struct TrainNode *next;
  struct TrainNode *last;
};

void trainTask() {
  int trainNum, reply = 0, src;
  Receive(&src, (char *)&trainNum, sizeof(int));
  Reply(src, (char *)&reply, sizeof(int));

  track_node track[TRACK_MAX];
  initTrack(track);

  int i, j;
  int stopBuffer[100];
  int stopHead = 0;
  struct TrainNode nodes[100];
  struct TrainNode *first = NULL;
  struct TrainNode *last = NULL;

  int numUpdates = 0;

  train_direction direction = DIR_FORWARD;

  int curSpeed = 0, maxSpeed = 0;
  int velocity[15];
  initVelocities(trainNum, velocity);

  acceleration_type accState = NONE;
  int accDist = 0, t0 = 0, v0 = 0;

  int location = -1, displayLocation = -1, delta = 0;

  Create(3, periodicTask);

  struct TrainMessage msg;
  int dest, sensorsPassed;
  int oldSensor, sensor, oldTime, time;

  int driver, curVelocity, v1;
  struct TrainNode *myNode;
  struct TrainDriverMessage driverMsg;
  int nextNode = -1, nextNodeDist = 0;
  char prevSwitchState = 'S';
  while(true) {
    Receive(&src, (char *)&msg, sizeof(struct TrainMessage));
    switch(msg.type) {
      case TRAINUPDATELOCATION:
	Reply(src, (char *)&reply, sizeof(int));
	float offset = velocity[curSpeed]*2;
	if(accState != NONE) {
	  offset = currentPosition(t0, v0, velocity[curSpeed], &accState);
	  offset -= accDist;
	  accDist += offset;
	}
	if(track[location].type != NODE_EXIT) {	//can't move past an exit
	  delta += offset;
	}

	v1 = (curSpeed == 0) ? 0 : velocity[curSpeed];
	if(currentVelocity(t0, v0, v1, &accState) == 0) {
	  for(i=0; i<stopHead; i++) {
	    Reply(stopBuffer[i], (char *)&reply, sizeof(int));
	  }
	  stopHead = 0;
	}

	//check if reaches the next node
	if(track[location].type == NODE_BRANCH) {
	  if(prevSwitchState == 'S') {
	    nextNode = getNodeIndex(track, ((track[location].edge[DIR_STRAIGHT]).dest));
	  }else{
	    nextNode = getNodeIndex(track, ((track[location].edge[DIR_CURVED]).dest));
	  }
	  nextNodeDist = adjDistance(&track[location], &track[nextNode]);
	}else if(track[location].type == NODE_EXIT) {
	  nextNode = location;
	  nextNodeDist = 0xFFFFFF;
	  if(direction == DIR_FORWARD) {
	    delta = -20000;
	  }else{
	    delta = -140000;
	  }
	}else{
	  nextNode = getNodeIndex(track, ((track[location].edge[DIR_AHEAD]).dest));
	  nextNodeDist = adjDistance(&track[location], &track[nextNode]);
	}
	if(delta >= nextNodeDist+200000 || 
	   (track[nextNode].type != NODE_SENSOR && delta >= nextNodeDist)) {
	  myNode = first;
	  while(myNode != NULL) {
	    if(location == myNode->location) {
	      myNode->location = nextNode;
	      myNode->delta -= nextNodeDist;
	    }
	    myNode = myNode->next;
	  }

	  delta -= nextNodeDist;
	  location = nextNode;
	  if(track[location].type == NODE_BRANCH) {
	    prevSwitchState = getSwitchState(track[location].num);
	  }
	}

	myNode = first;
	while(myNode != NULL) {
	  if(location == myNode->location && delta >= myNode->delta) {
	    Reply(myNode->src, (char *)&reply, sizeof(int));
	    if(myNode->next == NULL && myNode->last == NULL) {
	      first = last = NULL;
	    }else if(myNode->next == NULL) {
	      last = myNode->last;
	      last->next = NULL;
	    }else if(myNode->last == NULL) {
	      first = myNode->next;
	      first->last = NULL;
	    }else{
	      (myNode->last)->next = myNode->next;
	      (myNode->next)->last = myNode->last;
	    }
	  }
	  myNode = myNode->next;
	}

	++numUpdates;
	if(numUpdates == 10) {
	  printAt(8, 1, "\e[K%s + %dcm", track[location].name, delta/10000);
	  printAt(9, 1, "\e[KVelocity: %dmm/s", currentVelocity(t0, v0, v1, &accState)/10);
	  numUpdates = 0;
	}
	break;
      case TRAINGOTO:
	maxSpeed = msg.speed;
	curSpeed = 0;
	dest = 0;
	while(dest < TRACK_MAX && strcmp(track[dest].name, msg.dest) != 0) dest++;

	driverMsg.trainNum = trainNum;
	driverMsg.source = location;
	driverMsg.dest = dest;
	driverMsg.doReverse = msg.doReverse;
	for(i=0; i<16; i++) {
	  driverMsg.velocity[i] = velocity[i];
	}
	driver = Create(3, trainDriver);
	Send(driver, (char *)&driverMsg, sizeof(struct TrainDriverMessage), (char *)&reply, sizeof(int));
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAININIT:
	Reply(src, (char *)&reply, sizeof(int));
	Putc2(1, (char)2, (char)trainNum);
	location = waitOnAnySensor();
	displayLocation = location;
	Putc2(1, (char)0, (char)trainNum);
	delta = 50000;	//stop about 5cm past point of sensor
	break;
      case TRAINCONFIGVELOCITY:
	//set turnouts to create oval shaped loop
	setSwitchState(14, 'C');
	setSwitchState(13, 'S');
	setSwitchState(10, 'S');
	setSwitchState(9, 'C');
	setSwitchState(8, 'C');
	setSwitchState(17, 'S');
	setSwitchState(16, 'S');
	setSwitchState(15, 'C');
	for(i=8; i<15; i++) {
	  Putc2(1, (char)i, (char)trainNum);
	  oldSensor = waitOnAnySensor();
	  oldTime = Time();
	  sensorsPassed = 0;
	  for(j=0; j<15; j++) {
	    sensor = waitOnAnySensor();
	    time = Time();
	    int distance = 1000*BFS(oldSensor, sensor, track, NULL, false);
	    int newVelocity = distance/(time - oldTime);
	    if(velocity[i] - newVelocity < velocity[i]/200 && sensorsPassed >= 10) break;
	    velocity[i] *= 95;
	    velocity[i] += 5*newVelocity;
	    velocity[i] /= 100;
	    ++sensorsPassed;
	  }
	  printf("Configured speed %d to %dmm/s\r", i, velocity[i]/10);
	}
	Putc2(1, (char)0, (char)trainNum);
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINSETLOCATION:
	location = msg.location;
	displayLocation = location;
	delta = 0;
	if(track[location].type == NODE_BRANCH) {
	  prevSwitchState = getSwitchState(track[location].num);
	}
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINSETDISPLAYLOC:
	displayLocation = location;
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINWAITFORLOCATION:
	if(location == msg.location && delta >= msg.delta) {
	  Reply(src, (char *)&reply, sizeof(int));
	}else{
	  myNode = &nodes[src & 0x7F];
	  myNode->src = src;
	  myNode->location = msg.location;
	  myNode->delta = msg.delta;
	  myNode->next = NULL;
	  if(first == NULL) {
	    first = last = myNode;
	    myNode->last = NULL;
	  }else{
	    last->next = myNode;
	    myNode->last = last;
	    last = myNode;
	  }
	}
	break;
      case TRAINWAITFORSTOP:
	v1 = (curSpeed == 0) ? 0 : velocity[curSpeed];
	if(currentVelocity(t0, v0, v1, &accState) > 0) {
	  stopBuffer[stopHead++] = src;
	  stopHead %= 100;
	}else{	
	  Reply(src, (char *)&reply, sizeof(int));
	}
	break;
      case TRAINSETACC:
	v0 = 0;
	accState = ACCELERATING;
	t0 = Time();
	accDist = 0;
	curSpeed = maxSpeed;
	Putc2(1, (char)maxSpeed, (char)trainNum);
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINSETDEC:
	v0 = currentVelocity(t0, v0, velocity[curSpeed], &accState);
	accState = DECELERATING;
	t0 = Time();
	accDist = 0;
	curSpeed = 0;
	Putc2(1, (char)0, (char)trainNum);
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINGETLOCATION:
	msg.location = location;
	msg.delta = delta;
	Reply(src, (char *)&msg, sizeof(struct TrainMessage));
	break;
      case TRAINGETVELOCITY:
	curVelocity = currentVelocity(t0, v0, velocity[curSpeed], &accState);
	Reply(src, (char *)&curVelocity, sizeof(int));
	break;
      case TRAINGETMAXVELOCITY:
	curVelocity = velocity[maxSpeed];
	Reply(src, (char *)&curVelocity, sizeof(int));
	break;
      case TRAINREMOVETASK:
	myNode = first;
	while(myNode != NULL) {
	  if(myNode->src == msg.tid) {
	    if(myNode->last == NULL && myNode->next == NULL) {
	      first = last = NULL;
	    }else if(myNode->next == NULL) {
	      last = myNode->last;
	      last->next = NULL;
	    }else if(myNode->last == NULL) {
	      first = myNode->next;
	      first->last = NULL;
	    }else{
	      (myNode->last)->next = myNode->next;
	      (myNode->next)->last = myNode->last;
	    }
	  }
	  myNode = myNode->next;
	}
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINGETACCSTATE:
	Reply(src, (char *)&accState, sizeof(acceleration_type));
	break;
      case TRAINREVERSE:
	Reply(src, (char *)&reply, sizeof(int));
	Putc2(1, (char)15, (char)trainNum);
	direction = (direction == DIR_FORWARD) ? DIR_BACKWARD : DIR_FORWARD;
	location = getNodeIndex(track, track[location].reverse);
	printf("reversed into node %s\r", track[location].name);
	delta = -delta;
	delta += PICKUPLENGTH;
	if(track[location].type == NODE_BRANCH) {
	  prevSwitchState = getSwitchState(track[location].num);
	}
	break;
      case TRAINGETDIR:
	Reply(src, (char *)&direction, sizeof(train_direction));
	break;
    }
  } 
}

void removeTrainTask(int trainTid, int taskID) {
  struct TrainMessage msg;
  msg.type = TRAINREMOVETASK;
  msg.tid = taskID;
  int reply;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

void setTrainLocation(int trainTid, int location, int delta) {
  struct TrainMessage msg;
  msg.type = TRAINSETLOCATION;
  msg.location = location;
  msg.delta = delta;
  int reply;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

void setTrainDisplayLocation(int trainTid, int location) {
  struct TrainMessage msg;
  msg.type = TRAINSETDISPLAYLOC;
  msg.location = location;
  int reply;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

void waitForLocation(int trainTid, int location, int delta) {
  struct TrainMessage msg;
  msg.type = TRAINWAITFORLOCATION;
  msg.location = location;
  msg.delta = delta;
  int reply;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

void waitForStop(int trainTid) {
  struct TrainMessage msg;
  msg.type = TRAINWAITFORSTOP;
  int reply;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

void setAccelerating(int trainTid) {
  struct TrainMessage msg;
  msg.type = TRAINSETACC;
  int reply;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

void setDecelerating(int trainTid) {
  struct TrainMessage msg;
  msg.type = TRAINSETDEC;
  int reply;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

int getTrainLocation(int trainTid, int *location) {
  struct TrainMessage msg, reply;
  msg.type = TRAINGETLOCATION;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(struct TrainMessage));

  *location = reply.location;
  return reply.delta;
}

int getTrainVelocity(int trainTid) {
  struct TrainMessage msg;
  msg.type = TRAINGETVELOCITY;
  int velocity;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&velocity, sizeof(int));

  return velocity;
}

int getTrainMaxVelocity(int trainTid) {
  struct TrainMessage msg;
  msg.type = TRAINGETMAXVELOCITY;
  int velocity;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&velocity, sizeof(int));

  return velocity;
}

acceleration_type getAccState(int trainTid) {
  struct TrainMessage msg;
  msg.type = TRAINGETACCSTATE;
  acceleration_type accState;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&accState, sizeof(acceleration_type));

  return accState;
}

void reverseTrain(int trainTid) {
  struct TrainMessage msg;
  msg.type = TRAINREVERSE;
  int reply;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

train_direction getTrainDirection(int trainTid) {
  struct TrainMessage msg;
  msg.type = TRAINGETDIR;
  train_direction direction;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&direction, sizeof(train_direction));

  return direction;
}

int getNodeIndex(track_node *track, track_node *location) {
  return location - track;
  /*switch(location->type) {
    case NODE_SENSOR:
      return locaton->num;
      break;
    case NODE_BRANCH:
      return 2*(location->num) + 78;
      break;
    case NODE_MERGE:
      return 2*(location->num) + 79;
      break;
    case NODE_ENTER:
      break;
    case NODE_EXIT:
      
      break;
    default:
      return -1;
      break;
  }*/
}

int getNextStop(struct Path *path, int curNode) {
  curNode++;
  while(curNode < path->numNodes) {
    if(curNode == (path->numNodes)-1) return curNode;

    if(((path->node)[curNode])->reverse == (path->node)[curNode+1]) return curNode;

    ++curNode;
  }
  return -1;
}

int getNextSwitch(struct Path *path, int curNode) {
  curNode++;
  while(curNode < (path->numNodes)-1) {
    if(((path->node)[curNode])->type == NODE_BRANCH) return curNode;

    ++curNode;
  }
  return -1;
}

int getNextSensor(struct Path *path, int curNode) {
  curNode++;
  while(curNode < (path->numNodes)-1) {
    if(((path->node)[curNode])->type == NODE_SENSOR) return curNode;

    ++curNode;
  }
  return -1;
}

void getAllBranchMissSensors(struct Path *path, int curNode, int *sensors, int *switches) {
  int curSensor = 0;

  curNode++;
  while(curNode < (path->numNodes)-1 && ((path->node)[curNode])->type != NODE_SENSOR) {
    if(((path->node)[curNode])->type == NODE_BRANCH) {
      switches[curSensor] = ((path->node)[curNode])->num;
      int direction = adjDirection((path->node)[curNode], (path->node)[curNode+1]);
      direction = (direction == DIR_STRAIGHT) ? DIR_CURVED : DIR_STRAIGHT;
      sensors[curSensor++] = getNextSensorForTrackState(((((path->node)[curNode])->edge)[direction]).dest);
    }

    ++curNode;
  }
}

int getNextSensorForTrackState(track_node *node) {
  while(node->type != NODE_SENSOR) {
    if(node->type == NODE_BRANCH) {
      char direction = getSwitchState(node->num);
      if(direction == 'S') {
	node = ((node->edge)[DIR_STRAIGHT]).dest;
      }else{
	node = ((node->edge)[DIR_CURVED]).dest;
      }
    }else if(node->type == NODE_EXIT) {
      return -1;
    }else{
      node = ((node->edge)[DIR_AHEAD]).dest;
    }
  }

  return node->num;
}
      

struct SwitchMessage {
  int location;
  int delta;
  int switchNum;
  int direction;
  int done;
};

struct NodeMessage {
  int location;
  int reverseNode;
  int done;
};

struct SensorMessage {
  int sensorNum;
  int sensorMiss;
  int switchMiss[3];
  int done;
};

void switchWatcher() {
  int message = SWITCHDONE;
  struct SwitchMessage switchInfo;
  int trainTid, src;
  Receive(&src, (char *)&trainTid, sizeof(int));
  Reply(src, (char *)&trainTid, sizeof(int));

  int driver = MyParentTid();
  while(true) {
    Send(driver, (char *)&message, sizeof(int), (char *)&switchInfo, sizeof(struct SwitchMessage));
    if(switchInfo.done) break;

    waitForLocation(trainTid, switchInfo.location, switchInfo.delta);
    if(switchInfo.direction == DIR_STRAIGHT) {
      setSwitchState(switchInfo.switchNum, 'S');
    }else{
      setSwitchState(switchInfo.switchNum, 'C');
    }
  }
  Destroy(MyTid());
}

void nodeWatcher() {
  int message = NODEDONE;
  struct NodeMessage nodeInfo;
  int trainTid, src;
  Receive(&src, (char *)&trainTid, sizeof(int));
  Reply(src, (char *)&trainTid, sizeof(int));

  int driver = MyParentTid();
  while(true) {
    Send(driver, (char *)&message, sizeof(int), (char *)&nodeInfo, sizeof(struct NodeMessage));
    if(nodeInfo.done) break;

    if(nodeInfo.reverseNode) {
      waitForStop(trainTid);
      reverseTrain(trainTid);
      setAccelerating(trainTid);
    }

    waitForLocation(trainTid, nodeInfo.location, 0);
  }
  Destroy(MyTid());
}

void sensorWatcher() {
  int message = SENSORDONE;
  struct SensorMessage sensorInfo;
  int trainTid, src;
  Receive(&src, (char *)&trainTid, sizeof(int));
  Reply(src, (char *)&trainTid, sizeof(int));

  int driver = MyParentTid(), i;
  struct SensorStates sensors;
  while(true) {
    Send(driver, (char *)&message, sizeof(int), (char *)&sensorInfo, sizeof(struct SensorMessage));
    if(sensorInfo.done) break;

    sensors.stateInfo[0] = 0;
    sensors.stateInfo[1] = 0;
    sensors.stateInfo[2] = 0;
    setSensor(&sensors, sensorInfo.sensorNum, true);
    if(sensorInfo.sensorMiss != -1) {
      setSensor(&sensors, sensorInfo.sensorMiss, true);
    }
    for(i=0; i<3; i++) {
      if((sensorInfo.switchMiss)[i] != -1) {
        setSensor(&sensors, (sensorInfo.switchMiss)[i], true);
      }
    }

    int sensorHit = waitOnSensors(&sensors);
    if(sensorHit == sensorInfo.sensorNum) {
      message = SENSORDONE;
    }else if(sensorHit == sensorInfo.sensorMiss) {
      message = SENSORMISSED;
    }else if(sensorHit == (sensorInfo.switchMiss)[0]) {
      message = SWITCH0MISSED;
    }else if(sensorHit == (sensorInfo.switchMiss)[1]) {
      message = SWITCH1MISSED;
    }else if(sensorHit == (sensorInfo.switchMiss)[2]) {
      message = SWITCH2MISSED;
    }
  }
}

void trainDriver() {
  struct TrainDriverMessage msg;
  int src, reply = 0;
  Receive(&src, (char *)&msg, sizeof(struct TrainDriverMessage));
  Reply(src, (char *)&reply, sizeof(int));

  track_node track[TRACK_MAX];
  initTrack(track);

  struct Path path;
  shortestPath(msg.source, msg.dest, track, &path, msg.doReverse);
  if((path.node[path.numNodes-2])->reverse == path.node[path.numNodes-1]) {
    path.numNodes--;
  }

  //initailize workers
  int trainTid = MyParentTid();
  int periodic = Create(2, periodicTask);
  int noder = Create(2, nodeWatcher);
  int switcher = Create(2, switchWatcher);
  int sensorer = Create(2, sensorWatcher);
  Send(noder, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));
  Send(switcher, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));
  Send(sensorer, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));

  int messageType;
  int pathNode;
  int stopLength, switchDistance;
  int curNode = 1, curSwitch = getNextSwitch(&path, curNode-1);
  int curSensor = getNextSensor(&path, curNode-1), curStop = getNextStop(&path, curNode-1);
  int firstNode = curNode, firstSensor = curSensor;
  struct SwitchMessage switchMsg;
  struct NodeMessage nodeMsg;
  struct SensorMessage sensorMsg;

  int stopNode, stopDistance, location, delta;

  int tasksComplete = 0, lastSensor = -1;
  int brokenSwitchNum[3];
  char switchState;
  setAccelerating(trainTid);
  while(tasksComplete < 4) {
    Receive(&src, (char *)&messageType, sizeof(int));
    
    switch(messageType) {
      case CHECKSTOP:
	if(curStop == -1) {
	  ++tasksComplete;
	  printf("stopping done\r");
	  break;
	}
	Reply(src, (char *)&reply, sizeof(int));
	stopLength = stoppingDistance(getTrainVelocity(trainTid));
	if(getTrainDirection(trainTid) == DIR_FORWARD) {
	  stopLength += 20000;
	}else{
	  stopLength += 140000;
	}
	if(curStop != path.numNodes-1) {
	  stopLength -= (REVERSEOVERSHOOT+200000);
	}
	stopNode = distanceBefore(&path, stopLength, curStop, &stopDistance);
	delta = getTrainLocation(trainTid, &location);
	if((location == getNodeIndex(track, path.node[stopNode]) && delta >= stopDistance) ||
	   location == getNodeIndex(track, path.node[stopNode+1])) {
	  setDecelerating(trainTid);
	  curStop = getNextStop(&path, curStop);
	}
	break;
      case SWITCHDONE:
	if(curSwitch == -1) {
	  ++tasksComplete;
	  switchMsg.done = true;
	  Reply(src, (char *)&switchMsg, sizeof(struct SwitchMessage));
	  printf("switching done\r");
	  break;	//reached last switch in path
	}
	switchDistance = getTrainMaxVelocity(trainTid)*40;
	if(getTrainDirection(trainTid) == DIR_BACKWARD) {
	  switchDistance += 140000;
	}
	pathNode = distanceBefore(&path, switchDistance, curSwitch, &switchMsg.delta);
	switchMsg.location = getNodeIndex(track, path.node[pathNode]);
	switchMsg.switchNum = (path.node[curSwitch])->num;
	switchMsg.direction = adjDirection(path.node[curSwitch], path.node[curSwitch+1]);
	switchMsg.done = false;
	Reply(src, (char *)&switchMsg, sizeof(struct SwitchMessage));
	curSwitch = getNextSwitch(&path, curSwitch);
	break;
      case NODEDONE:
	if(curNode == path.numNodes-1) {
	  ++tasksComplete;
	  nodeMsg.done = true;
	  Reply(src, (char *)&nodeMsg, sizeof(struct NodeMessage));
	  printf("nodes done\r");
	  break;
	}

	nodeMsg.location = getNodeIndex(track, path.node[curNode]);
	if((path.node[curNode-1])->reverse == path.node[curNode]) {
	  nodeMsg.reverseNode = true;
	}else{
	  nodeMsg.reverseNode = false;
	}
	nodeMsg.done = false;
	Reply(src, (char *)&nodeMsg, sizeof(struct NodeMessage));
	curNode++;
	break;
      case SENSORMISSED:
	printColored(RED, BLACK, "sensor %s failed to trigger\r", (path.node[lastSensor])->name);
	lastSensor = curSensor;
	curSensor = getNextSensor(&path, curSensor);
      case SENSORDONE:
	if(curSensor != firstSensor) {
	  delta = getTrainLocation(trainTid, &location);
	  int dist = distanceAlongPath(&path, &track[location], path.node[lastSensor]);
	  int err = delta - dist;
	  printf("distance error at sensor %s is %dmm\r", (path.node[lastSensor])->name, err/1000);

	  setTrainLocation(trainTid, getNodeIndex(track, path.node[lastSensor]), 0);
	}

	if(curSensor == -1 || curSensor == path.numNodes-1) {
	  ++tasksComplete;
	  sensorMsg.done = true;
	  Reply(src, (char *)&sensorMsg, sizeof(struct SensorMessage));
	  printf("sensors done\r");
	  break;
	}

	sensorMsg.sensorNum = getNodeIndex(track, path.node[curSensor]);
	sensorMsg.switchMiss[0] = sensorMsg.switchMiss[1] = sensorMsg.switchMiss[2] = -1;
	if(lastSensor != -1) {
	  getAllBranchMissSensors(&path, lastSensor, sensorMsg.switchMiss, brokenSwitchNum);
	}

	lastSensor = curSensor;
	curSensor = getNextSensor(&path, curSensor);
	if(curSensor != -1) {
	  sensorMsg.sensorMiss = getNodeIndex(track, path.node[curSensor]);
	}else{
	  sensorMsg.sensorMiss = -1;
	}
	sensorMsg.done = false;
	Reply(src, (char *)&sensorMsg, sizeof(struct SensorMessage));
	break;
      case SWITCH0MISSED:
      case SWITCH1MISSED:
      case SWITCH2MISSED:
	setTrainLocation(trainTid, sensorMsg.switchMiss[messageType-SWITCH0MISSED], 0);
	if(getAccState(trainTid) != DECELERATING) {
	  setDecelerating(trainTid);
	}
	printColored(RED, BLACK, "turnout BR%d failed to switch\r", brokenSwitchNum[messageType-SWITCH0MISSED]);
	switchState = getSwitchState(brokenSwitchNum[messageType-SWITCH0MISSED]);
	switchState = (switchState == 'S') ? 'C' : 'S';
	setSwitchState(brokenSwitchNum[messageType-SWITCH0MISSED], switchState);
	waitForStop(trainTid);

	//Destroy all helper tasks;
	sensorMsg.done = true;
	Reply(src, (char *)&sensorMsg, sizeof(struct SensorMessage));
	Destroy(noder);
	removeTrainTask(trainTid, noder);
	Destroy(switcher);
	removeTrainTask(trainTid, switcher);

	//Calculate new path
	delta = getTrainLocation(trainTid, &location);
  	shortestPath(location, msg.dest, track, &path, msg.doReverse);
  	if((path.node[path.numNodes-2])->reverse == path.node[path.numNodes-1]) {
  	  path.numNodes--;
  	}

	//Create new helper tasks
        noder = Create(2, nodeWatcher);
  	switcher = Create(2, switchWatcher);
  	sensorer = Create(2, sensorWatcher);
  	Send(noder, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));
  	Send(switcher, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));
  	Send(sensorer, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));

	//re-initialize variables
  	curNode = 1;
	curSwitch = getNextSwitch(&path, curNode-1);
  	curSensor = getNextSensor(&path, curNode-1);
	curStop = getNextStop(&path, curNode-1);
  	firstNode = curNode;
	firstSensor = curSensor;
  	tasksComplete = 0;
	lastSensor = -1;

	setAccelerating(trainTid);
	break;
    }
  }

  waitForStop(trainTid);
  if((path.node[path.numNodes-1])->type == NODE_SENSOR) {
    int src, reply;
    int timeoutTask = Create(3, delayTask);
    int delayTime = 100;
    Send(timeoutTask, (char *)&delayTime, sizeof(int), (char *)&reply, sizeof(int));

    int sensorTask = Create(3, sensorWaitTask);
    int sensorNum = (path.node[path.numNodes-1])->num;
    Send(sensorTask, (char *)&sensorNum, sizeof(int), (char *)&reply, sizeof(int));

    Receive(&src, (char *)&reply, sizeof(int));
    if(src == timeoutTask) {
      Destroy(timeoutTask);
      Destroy(sensorTask);
      removeSensorTask(sensorTask);
    }else{
      Destroy(sensorTask);
      setTrainLocation(trainTid, sensorNum, 0);
      Receive(&src, (char *)&reply, sizeof(int));
      Destroy(timeoutTask);
    }
  }
  printf("reached destination\r");

  Receive(&src, (char *)&reply, sizeof(int));
  reply = PERIODICSTOP;
  Reply(src, (char *)&reply, sizeof(int));
  Destroy(MyTid());
}

void moveToLocation(int trainNum, int source, int dest, track_node *track, int doReverse, int *velocity) {
  struct Path path;

  BFS(source, dest, track, &path, doReverse);
  if((path.node[path.numNodes-2])->reverse == path.node[path.numNodes-1]) {
    path.numNodes--;
  }

  //TODO this obviously isn't permanent
  int i;
  for(i=0; i<path.numNodes-1; i++) {
    if((path.node[i])->type == NODE_BRANCH) {
      if(adjDirection(path.node[i], path.node[i+1]) == DIR_STRAIGHT) {
	setSwitchState((path.node[i])->num, 'S');
      }else{
	setSwitchState((path.node[i])->num, 'C');
      }
    }
  }

  int curNode = 1, trainTid = MyParentTid();

  setAccelerating(trainTid);

  while(curNode < path.numNodes) {
    if((path.node[curNode-1])->reverse == path.node[curNode]) {
      waitForStop(trainTid);
      if(curNode != 1) {
	setTrainDisplayLocation(trainTid, getNodeIndex(track, path.node[curNode]));
	setTrainLocation(trainTid, getNodeIndex(track, path.node[curNode-1]), -300000);
      }
      Putc2(1, (char)15, (char)trainNum);
      setAccelerating(trainTid);
    }

    int distance = adjDistance(path.node[curNode-1], path.node[curNode]);
    if(curNode == 1 && (path.node[0])->reverse == path.node[1]) {
    }else if((path.node[curNode])->type == NODE_SENSOR) {
      waitOnSensor((path.node[curNode])->num);
      int location;
      int delta = getTrainLocation(trainTid, &location);
      int err = delta/1000 - distance/1000;
      printf("distance error at node %s: %dmm\r", (path.node[curNode])->name, err);
    }else{
      waitForLocation(trainTid, getNodeIndex(track, path.node[curNode-1]), distance);
    }

    setTrainLocation(trainTid, getNodeIndex(track, path.node[curNode]), 0);
    curNode++;
  }
  setDecelerating(trainTid);
}

int distanceAlongPath(struct Path *path, track_node *node1, track_node *node2) {
  int dist = 0;

  track_node *source;
  track_node *dest;
  int curNode = 0;
  while((path->node)[curNode] != node1 && (path->node)[curNode] != node2) {
    if(curNode == path->numNodes-1) return -1;
    curNode++;
  }
  if((path->node)[curNode] == node1) {
    source = node1;
    dest = node2;
  }else{
    dest = node1;
    source = node2;
  }

  while((path->node)[curNode] != dest) {
    if(curNode == path->numNodes-1) return -1;
    dist += adjDistance((path->node)[curNode], (path->node)[curNode+1]);
    curNode++;
  }
  return dist;
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
      case '\n':
	//ignore newlines
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
