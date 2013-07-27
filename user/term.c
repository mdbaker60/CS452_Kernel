#include <bwio.h>
#include <term.h>
#include <string.h>
#include <syscall.h>
#include <io.h>
#include <values.h>
#include <clockServer.h>
#include <trackServer.h>
#include <track.h>
#include <prng.h>
#include <velocity.h>
#include <math.h>
#include <train.h>

#define NUMCOMMANDS 	13
#define MAX_ARGS	25

struct WanderMessage {
  int trainTid;
  int numLocations;
  int doReverse;
};

void WanderTask() {
  struct WanderMessage msg;
  int src, reply;
  Receive(&src, (char *)&msg, sizeof(struct WanderMessage));
  Reply(src, (char *)&reply, sizeof(int));

  track_node track[TRACK_MAX];
  initTrack(track);
  int trackSize = (getTrack() == TRACKA) ? NUM_TRACKA : NUM_TRACKB;

  int i;
  struct TrainMessage trainMsg;
  trainMsg.type = TRAINGOTO;
  struct PRNG prng_mem;
  struct PRNG *prng = &prng_mem;
  seed(prng, Time());

  int trainColor = getTrainColor(msg.trainTid);

  for(i=0; i<msg.numLocations; i++) {
    trainMsg.doReverse = msg.doReverse;
    trainMsg.speed = 12;
    int dest = random(prng)%trackSize;
    strcpy(trainMsg.dest, track[dest].name);
    printColored(trainColor, BLACK, "Moving to %s(destination %d)\r", trainMsg.dest, i+1);
    Send(msg.trainTid, (char *)&trainMsg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
  }

  Send(MyParentTid(), (char *)&src, sizeof(int), (char *)&reply, sizeof(int));
  Destroy(MyTid());
}

char *splitCommand(char *command);

void CleanShutdown() {
  DSShutdown();
  bwprintf(COM2, "\e[1;1f\e[2J");
  Shutdown();
}

int largestPrefix(char *str1, char *str2) {
  int i=0;
  while(str1[i] != '\0' && str1[i] == str2[i]) i++;
  return i;
}

int tabComplete(char *command) {
  char *commands[NUMCOMMANDS] = {"q", "tr", "rv", "sw", "selectTrack", "move", "randomizeSwitches", "init", "clear", "configureVelocities", "addTrain", "changeTrainColor", "wander"};

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
      i++;
      while(command[i] != '\0' && command[i] == ' ') i++;
      if(command[i] == '\0') {
	return command;
      }else{
        return command + i;
      }
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

void moveTrainTask() {
  struct TrainMessage msg;
  int src, reply, trainTid;
  Receive(&src, (char *)&msg, sizeof(struct TrainMessage));
  Reply(src, (char *)&reply, sizeof(int));
  Receive(&src, (char *)&trainTid, sizeof(int));
  Reply(src, (char *)&reply, sizeof(int));
  
  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
  Send(src, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));
  Destroy(MyTid());
}

void parseCommand(char *command, int *trainSpeeds, int *train, struct PRNG *prng) {
  char *argv[MAX_ARGS];
  int argc = formatArgs(command, argv);

  if(strcmp(argv[0], "tr") == 0) {
    if(numArgs(argc, argv) != 2) {
      printColored(RED, BLACK, "Usage: tr train_number train_speed\r");
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
      printColored(RED, BLACK, "Usage: rv train_number\r");
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
      printColored(RED, BLACK, "Usage: sw switch_number switch_state\r");
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
    CleanShutdown();
  }else if(strcmp(argv[0], "selectTrack") == 0) {
    if(numArgs(argc, argv) != 1) {
      printColored(RED, BLACK, "Usage: selectTrack track_name\r");
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
    if(numArgs(argc, argv)%3 != 0) {
      printColored(RED, BLACK, "Usage: (move train_number destination speed)...\r");
    }else{
      int numTrains = numArgs(argc, argv)/3;
      int trainCommandTasks[6];
      struct TrainMessage msg;
      int src, reply;
      int i;
      for(i=0; i<numTrains; i++) {
	int trainNum = strToInt(getArgument(argc, argv, i*3));
	if(train[trainNum] == -1) {
	  printColored(RED, BLACK, "train %d was not added\r", trainNum);
	  continue;
	}
	trainCommandTasks[i] = Create(2, moveTrainTask);
	msg.type = TRAINGOTO;
	strcpy(msg.dest, getArgument(argc, argv, i*3+1));
	msg.doReverse = getFlag(argc, argv, "r");
	msg.speed = strToInt(getArgument(argc, argv, i*3+2));
	Send(trainCommandTasks[i], (char *)&msg, sizeof(struct TrainMessage),
	     (char *)&reply, sizeof(int));
	Send(trainCommandTasks[i], (char *)&train[trainNum], sizeof(int), 
	     (char *)&reply, sizeof(int));
      }

      for(i=0; i<numTrains; i++) {
	Receive(&src, (char *)&reply, sizeof(int));
	Reply(src, (char *)&reply, sizeof(int));
      }
    }
  }else if(strcmp(argv[0], "randomizeSwitches") == 0) {
    int i;
    for(i=1; i<19; i++) {
      int dir = random(prng) % 2;
      if(dir == 0) {
	setSwitchState(i, 'S');
      }else{
	setSwitchState(i, 'C');
      }
    }
    for(i=153; i<157; i++) {
      int dir = random(prng) % 2;
      if(dir == 0) {
	setSwitchState(i, 'S');
      }else{
	setSwitchState(i, 'C');
      }
    }
  }else if(strcmp(argv[0], "init") == 0) {
    if(numArgs(argc, argv) != 0) {
      printColored(RED, BLACK, "Usage: init\r");
    }else{
      struct TrainMessage msg;
      msg.type = TRAININIT;
      int trainNum, reply;
      for(trainNum = 0; trainNum<80; trainNum++) {
	if(train[trainNum] != -1) {
	  Send(train[trainNum], (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
	}
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
  }else if(strcmp(argv[0], "configureVelocities") == 0) {
    struct TrainMessage msg;
    msg.type = TRAINCONFIGVELOCITY;
    if(numArgs(argc, argv) != 0) {
      printColored(RED, BLACK, "Usage: configureVelocities\r");
    }else{
      int trainNum;
      int src, reply;

      //move all trains home
      int numTrains = 0;
      int trainCommandTasks[6];
      for(trainNum=0; trainNum<80; trainNum++) {
	if(train[trainNum] != -1) {
	  trainCommandTasks[numTrains] = Create(2, moveTrainTask);
	  msg.type = TRAINGOTO;
	  strcpy(msg.dest, getHomeFromTrainID(getTrainID(train[trainNum])));
	  msg.doReverse = false;
	  msg.speed = 10;
	  Send(trainCommandTasks[numTrains], (char *)&msg, sizeof(struct TrainMessage),
	       (char *)&reply, sizeof(int));
	  Send(trainCommandTasks[numTrains], (char *)&train[trainNum], sizeof(int), 
	       (char *)&reply, sizeof(int));
	  ++numTrains;
	}
      }
      int i;
      for(i=0; i<numTrains; i++) {
	Receive(&src, (char *)&reply, sizeof(int));
	Reply(src, (char *)&reply, sizeof(int));
      }

      //do configuration loop 1 by 1
      for(trainNum=0; trainNum<80; trainNum++) {
	msg.type = TRAINCONFIGVELOCITY;
	if(train[trainNum] != -1) {
          Send(train[trainNum], (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
	  printf("train %d done configuring\r", trainNum);
	}
      }
    }
  }else if(strcmp(argv[0], "addTrain") == 0) {
    if(numArgs(argc, argv) != 1) {
      printColored(RED, BLACK, "Usage: addTrain train_number\r");
    }else{
      int trainNum = strToInt(getArgument(argc, argv, 0));
      int reply;
      if(train[trainNum] != -1) {
	printColored(RED, BLACK, "Train %d already added\r", trainNum);
      }else{
	train[trainNum] = Create(5, trainTask);
	Send(train[trainNum], (char *)&trainNum, sizeof(int), (char *)&reply, sizeof(int));
      }
    }
  }else if(strcmp(argv[0], "changeTrainColor") == 0) {
    if(numArgs(argc, argv) != 2) {
      printColored(RED, BLACK, "Usage: changeTrainColor train_number train_color\r");
    }else{
      int trainNum = strToInt(getArgument(argc, argv, 0));
      if(train[trainNum] == -1) {
	printColored(RED, BLACK, "train %d has not been added yet\r", trainNum);
	return;
      }

      if(strcmp(getArgument(argc, argv, 1), "green") == 0) {
	setTrainColor(train[trainNum], GREEN);
      }else if(strcmp(getArgument(argc, argv, 1), "yellow") == 0) {
	setTrainColor(train[trainNum], YELLOW);
      }else if(strcmp(getArgument(argc, argv, 1), "blue") == 0) {
	setTrainColor(train[trainNum], BLUE);
      }else if(strcmp(getArgument(argc, argv, 1), "magenta") == 0) {
	setTrainColor(train[trainNum], MAGENTA);
      }else if(strcmp(getArgument(argc, argv, 1), "cyan") == 0) {
	setTrainColor(train[trainNum], CYAN);
      }else if(strcmp(getArgument(argc, argv, 1), "white") == 0) {
	setTrainColor(train[trainNum], WHITE);
      }else{
	printf("'%s' is not a valid color. valid colors are ", getArgument(argc, argv, 1));
	printColored(GREEN, BLACK, "green, ");
	printColored(YELLOW, BLACK, "yellow, ");
	printColored(BLUE, BLACK, "blue, ");
	printColored(MAGENTA, BLACK, "magenta, ");
	printColored(CYAN, BLACK, "cyan, ");
	printf("and white\r");
      }
    }
  }else if(strcmp(argv[0], "wander") == 0) {
    if(numArgs(argc, argv) < 2) {
      printColored(RED, BLACK, "Usage: wander train1_num... num_paths\r");
    }else{
      int numTrains = numArgs(argc, argv)-1;
      int i;
      int wanderTasks[6];
      int src, reply;
      struct WanderMessage msg;
      for(i=0; i<numTrains; i++) {
	wanderTasks[i] = Create(2, WanderTask);
	msg.trainTid = train[strToInt(getArgument(argc, argv, i))];
	msg.numLocations = strToInt(getArgument(argc, argv, numTrains));
	msg.doReverse = getFlag(argc, argv, "r");
	Send(wanderTasks[i], (char *)&msg, sizeof(struct WanderMessage), (char *)&reply, sizeof(int));
      }

      for(i=0; i<numTrains; i++) {
	Receive(&src, (char *)&reply, sizeof(int));
	Reply(src, (char *)&reply, sizeof(int));
      }
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

  struct PRNG prng;
  seed(&prng, Time());

  outputEscape("[2J[13;100r[37;40;0m");
  refreshScreen();
  initializeTrack();
  moveCursor(13, 1);
  printf(">");
  while(true) {
    c = (char)Getc(2);
    switch(c) {
      case '\r':
	curCommand[commandLength] = '\0';
	strcpy(commands[commandNum], curCommand);
	lengths[commandNum] = commandLength;
	printf("\r");
	parseCommand(curCommand, trainSpeeds, train, &prng);
	commandNum++;
	commandNum %= NUM_SAVED_COMMANDS;
	viewingNum = commandNum;
	commands[commandNum][0] = '\0';
	commandLength = 0;
	if(totalCommands < NUM_SAVED_COMMANDS-1) totalCommands++;
	printf(">");
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
	  printf("%c", c);
	}
        break;
    }
  }
}
