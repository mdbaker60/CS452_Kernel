#include <train.h>
#include <track_node.h>
#include <track.h>
#include <values.h>
#include <syscall.h>
#include <io.h>
#include <clockServer.h>
#include <trackServer.h>
#include <string.h>

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
      case NODE_NONE:
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

track_edge *adjEdge(track_node *src, track_node *dest) {
  if((src->edge)[0].dest == dest) {
    return &(src->edge)[0];
  }else if((src->edge)[1].dest == dest) {
    return &(src->edge)[1];
  }
  return NULL;
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

  int i;
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

  int myColor = WHITE;

  Create(3, periodicTask);

  struct TrainVelocityMessage msg;
  int dest;

  int termWaiting = -1, configuring = false;

  int driver, curVelocity, v1;
  struct TrainNode *myNode;
  struct TrainDriverMessage driverMsg;
  int nextNode = -1, nextNodeDist = 0;
  char prevSwitchState = 'S';
  while(true) {
    Receive(&src, (char *)&msg, sizeof(struct TrainVelocityMessage));
    switch(msg.type) {
      case TRAINUPDATELOCATION:
	Reply(src, (char *)&reply, sizeof(int));
	if(location == -1) break;
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
	  printColoredAt(myColor, BLACK, 8, 1, 
			 "\e[K%s + %dcm", track[location].name, delta/10000);
	  printColoredAt(myColor, BLACK, 9, 1, 
			 "\e[KVelocity: %dmm/s", currentVelocity(t0, v0, v1, &accState)/10);
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
	driver = Create(3, trainDriver);
	Send(driver, (char *)&driverMsg, sizeof(struct TrainDriverMessage), (char *)&reply, sizeof(int));
	termWaiting = src;
	break;
      case TRAININIT:
	Putc2(1, (char)2, (char)trainNum);
	location = waitOnAnySensor();
	Putc2(1, (char)0, (char)trainNum);
	delta = 50000;	//stop about 5cm past point of sensor

	//reserve the track at your current location
	blockOnReservation(getNodeIndex(track, track[location].reverse),
			   getNodeIndex(track, ((track[location].reverse)->edge)[DIR_AHEAD].dest));
	printColored(myColor, BLACK, "train %d located sensor %s\r", trainNum, track[location].name);
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINCONFIGVELOCITY:
	if(location == -1) break;
	//move to sensor B16
	maxSpeed = 8;
	curSpeed = 0;
	
	driverMsg.trainNum = trainNum;
	driverMsg.source = location;
	driverMsg.dest = 31;
	driverMsg.doReverse = false;
	driver = Create(3, trainDriver);
	Send(driver, (char *)&driverMsg, sizeof(struct TrainDriverMessage), (char *)&reply, sizeof(int));
	termWaiting = src;
	configuring = true;
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
	delta = -delta;
	delta += PICKUPLENGTH;
	if(track[location].type == NODE_BRANCH) {
	  prevSwitchState = getSwitchState(track[location].num);
	}
	break;
      case TRAINGETDIR:
	Reply(src, (char *)&direction, sizeof(train_direction));
	break;
      case TRAINSETCOLOR:
	myColor = msg.color;
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINGETCOLOR:
	Reply(src, (char *)&myColor, sizeof(int));
	break;
      case TRAINDRIVERDONE:
	if(configuring) {
	  driverMsg.trainNum = trainNum;
	  for(i=0; i<15; i++) {
	    driverMsg.velocity[i] = velocity[i];
	  }
	  int configer = Create(2, trainConfiger);
	  Send(configer, (char *)&driverMsg, sizeof(struct TrainDriverMessage), (char *)&reply, sizeof(int));
	  configuring = false;
	}else if(termWaiting != -1) {
	  Reply(termWaiting, (char *)&reply, sizeof(int));
	  termWaiting = -1;
	}
	Reply(src, (char *)&reply, sizeof(int));
	break;
      case TRAINCONFIGDONE:
	for(i=0; i<15; i++) {
	  velocity[i] = msg.velocity[i];
	}

	if(termWaiting != -1) {
	  Reply(termWaiting, (char *)&reply, sizeof(int));
	  termWaiting = -1;
	}
	Reply(src, (char *)&reply, sizeof(int));	
	break;
      case TRAINSETSPEED:
	maxSpeed = msg.speed;
	Reply(src, (char *)&reply, sizeof(int));	
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

void setTrainColor(int trainTid, int color) {
  struct TrainMessage msg;
  msg.type = TRAINSETCOLOR;
  msg.color = color;
  int reply;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

int getTrainColor(int trainTid) {
  struct TrainMessage msg;
  msg.type = TRAINGETCOLOR;
  int color;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&color, sizeof(int));

  return color;
}

void setTrainSpeed(int trainTid, int speed) {
  struct TrainMessage msg;
  msg.type = TRAINSETSPEED;
  msg.speed = speed;
  int reply;

  Send(trainTid, (char *)&msg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));
}

int getNodeIndex(track_node *track, track_node *location) {
  return location - track;
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

struct ReserveMessage {
  int location;
  int delta;
  int node1;
  int node2;
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
  Destroy(MyTid());
}

void reserveWatcher() {
  int message = RESERVEDONE;
  struct ReserveMessage reserveInfo;
  int trainTid, src;
  Receive(&src, (char *)&trainTid, sizeof(int));
  Reply(src, (char *)&trainTid, sizeof(int));

  int driver = MyParentTid();
  while(true) {
    Send(driver, (char *)&message, sizeof(int), (char *)&reserveInfo, sizeof(struct ReserveMessage));
    if(reserveInfo.done) break;

    waitForLocation(trainTid, reserveInfo.location, reserveInfo.delta);
    int gotReservation = getReservation(reserveInfo.node1, reserveInfo.node2);
    if(!gotReservation) {
      printColored(getTrainColor(trainTid), BLACK, "waiting for track to become available\r");
      setDecelerating(trainTid);
      blockOnReservation(reserveInfo.node1, reserveInfo.node2);
      setAccelerating(trainTid);
    }
  }
  Destroy(MyTid());
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

  int numReserved = 0, numUnreserved = 0;

  //initailize workers
  int trainTid = MyParentTid();
  Create(2, periodicTask);
  int noder = Create(2, nodeWatcher);
  int switcher = Create(2, switchWatcher);
  int sensorer = Create(2, sensorWatcher);
  int reserver = Create(2, reserveWatcher);
  Send(noder, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));
  Send(switcher, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));
  Send(sensorer, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));
  Send(reserver, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));

  int trainColor = getTrainColor(trainTid);

  int messageType;
  int pathNode;
  int stopLength, switchDistance;
  int curNode = 1, curSwitch = getNextSwitch(&path, curNode-1);
  int curSensor = getNextSensor(&path, curNode-1), curStop = getNextStop(&path, curNode-1);
  int curReserve = 1;
  int firstNode = curNode, firstSensor = curSensor;
  struct SwitchMessage switchMsg;
  struct NodeMessage nodeMsg;
  struct SensorMessage sensorMsg;
  struct ReserveMessage reserveMsg;

  int stopNode, stopDistance, location, delta;

  int tasksComplete = 0, lastSensor = -1;
  int brokenSwitchNum[3];
  char switchState;
  setAccelerating(trainTid);
  while(tasksComplete < 5) {
    Receive(&src, (char *)&messageType, sizeof(int));
    
    switch(messageType) {
      case CHECKSTOP:
	if(curStop == -1) {
	  ++tasksComplete;
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
	  break;
	}

	if(curNode != firstNode) {
	  returnReservation(getNodeIndex(track, path.node[curNode-2]), 
			    getNodeIndex(track, path.node[curNode-1]));
	}else{
	  if((path.node[0])->type == NODE_MERGE) {
	    char resDir = getSwitchState((path.node[0])->num);
	    if(resDir == 'S') {
	      returnReservation(getNodeIndex(track, (path.node[0])->reverse),
				getNodeIndex(track, (((path.node[0])->reverse)->edge)[DIR_STRAIGHT].dest));
	    }else{
	      returnReservation(getNodeIndex(track, (path.node[0])->reverse),
				getNodeIndex(track, (((path.node[0])->reverse)->edge)[DIR_CURVED].dest));
	    }
	  }else{
 	    returnReservation(getNodeIndex(track, (path.node[0])->reverse),
			      getNodeIndex(track, (((path.node[0])->reverse)->edge)[DIR_AHEAD].dest));
	  }
	}
	numUnreserved++;

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
	printColored(trainColor, RED, "sensor %s failed to trigger\r", (path.node[lastSensor])->name);
	lastSensor = curSensor;
	curSensor = getNextSensor(&path, curSensor);
      case SENSORDONE:
	if(curSensor != firstSensor) {
	  delta = getTrainLocation(trainTid, &location);
	  int dist = distanceAlongPath(&path, &track[location], path.node[lastSensor]);
	  int err = delta - dist;
	  printColored(trainColor, BLACK, 
		       "distance error at sensor %s is %dmm\r", 
		       (path.node[lastSensor])->name, err/1000);

	  setTrainLocation(trainTid, getNodeIndex(track, path.node[lastSensor]), 0);
	}

	if(curSensor == -1 || curSensor == path.numNodes-1) {
	  ++tasksComplete;
	  sensorMsg.done = true;
	  Reply(src, (char *)&sensorMsg, sizeof(struct SensorMessage));
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
	printColored(trainColor, RED, "turnout BR%d failed to switch\r", 
		     brokenSwitchNum[messageType-SWITCH0MISSED]);
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
	Destroy(reserver);
	removeTrainTask(trainTid, reserver);

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
	reserver = Create(2, reserveWatcher);
  	Send(noder, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));
  	Send(switcher, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));
  	Send(sensorer, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));
  	Send(reserver, (char *)&trainTid, sizeof(int), (char *)&reply, sizeof(int));

	//re-initialize variables
  	curNode = 1;
	curSwitch = getNextSwitch(&path, curNode-1);
  	curSensor = getNextSensor(&path, curNode-1);
	curStop = getNextStop(&path, curNode-1);
	curReserve = 1;
  	firstNode = curNode;
	firstSensor = curSensor;
  	tasksComplete = 0;
	lastSensor = -1;

	setAccelerating(trainTid);
	break;
      case RESERVEDONE:
	if(curReserve >= path.numNodes-1) {
	  ++tasksComplete;
	  reserveMsg.done = true;
	  Reply(src, (char *)&reserveMsg, sizeof(struct ReserveMessage));
	  break;
	}
	numReserved++;

	pathNode = distanceBefore(&path, 
				  stoppingDistance(getTrainMaxVelocity(trainTid))+100000,
				  curReserve, &reserveMsg.delta);
	reserveMsg.location = getNodeIndex(track, path.node[pathNode]);
	reserveMsg.node1 = getNodeIndex(track, path.node[curReserve]);
	reserveMsg.node2 = getNodeIndex(track, path.node[curReserve+1]);
	reserveMsg.done = false;
	Reply(src, (char *)&reserveMsg, sizeof(struct ReserveMessage));
	curReserve++;
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

  printColored(trainColor, BLACK, "reached destination\r");
  printf("made %d reservation, returned %d reservations\r", numReserved, numUnreserved);
  struct TrainMessage trainMsg;
  trainMsg.type = TRAINDRIVERDONE;
  Send(trainTid, (char *)&trainMsg, sizeof(struct TrainMessage), (char *)&reply, sizeof(int));

  Receive(&src, (char *)&reply, sizeof(int));
  reply = PERIODICSTOP;
  Reply(src, (char *)&reply, sizeof(int));
  Destroy(MyTid());
}

void trainConfiger() {
  struct TrainDriverMessage msg;
  struct TrainVelocityMessage trainMsg;
  int src, reply;
  Receive(&src, (char *)&msg, sizeof(struct TrainDriverMessage));
  Reply(src, (char *)&reply, sizeof(int));

  int sensor, oldSensor, time, oldTime, sensorsPassed;
  int i, j;

  track_node track[TRACK_MAX];
  initTrack(track);

  setSwitchState(14, 'C');
  setSwitchState(13, 'S');
  setSwitchState(10, 'S');
  setSwitchState(9, 'C');
  setSwitchState(8, 'C');
  setSwitchState(17, 'S');
  setSwitchState(16, 'S');
  setSwitchState(15, 'C');
  setTrainSpeed(MyParentTid(), 8);
  setAccelerating(MyParentTid());
  //wait for acceleration to completeish
  Delay(800);
  for(i=8; i<15; i++) {
    setTrainSpeed(MyParentTid(), i);
    Putc2(1, (char)i, (char)msg.trainNum);
    oldSensor = waitOnAnySensor();
    oldTime = Time();
    sensorsPassed = 0;
    for(j=0; j<15; j++) {
      sensor = waitOnAnySensor();
      setTrainLocation(MyParentTid(), sensor, 0);
      time = Time();
      int distance = 1000*BFS(oldSensor, sensor, track, NULL, false);
      int newVelocity = distance/(time - oldTime);
      if(msg.velocity[i] - newVelocity < msg.velocity[i]/200 && sensorsPassed >= 10) break;
      msg.velocity[i] *= 95;
      msg.velocity[i] += 5*newVelocity;
      msg.velocity[i] /= 100;
      ++sensorsPassed;
    }
    printf("Configured speed %d to %dmm/s\r", i, msg.velocity[i]/10);
    trainMsg.velocity[i] = msg.velocity[i];
  }
  setDecelerating(MyParentTid());

  trainMsg.type = TRAINCONFIGDONE;
  Send(MyParentTid(), (char *)&trainMsg, sizeof(struct TrainVelocityMessage), (char *)&reply, sizeof(int));
  Destroy(MyTid());
}
