#ifndef __TRAIN_H__
#define __TRAIN_H__

#include <track_node.h>
#include <velocity.h>

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
#define TRAINSETCOLOR		17
#define TRAINGETCOLOR		18
#define TRAINDRIVERDONE		19
#define TRAINCONFIGDONE		20
#define TRAINSETSPEED		21
#define TRAINGETID		22
#define TRAINGETNUM		23

#define CHECKSTOP		0
#define SWITCHDONE		1
#define NODEDONE		2
#define SENSORDONE		3
#define SENSORMISSED		4
#define SWITCH0MISSED		5
#define SWITCH1MISSED		6
#define SWITCH2MISSED		7
#define RESERVEDONE		8
#define RESERVETIMEOUT		9
#define RESERVENEEDSTOP		10
#define UNRESERVEDONE		11

#define PERIODICSTOP		2

#define TIMEOUTLENGTH		100000
#define REVERSEOVERSHOOT	100000
#define PICKUPLENGTH		48000


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

struct UnreserveMessage {
  int location;
  int delta;
  int node1;
  int node2;
  int done;
};

struct TrainDriverMessage {
  int trainNum;
  int source;
  int dest;
  int doReverse;
  int velocity[15];
};

struct TrainMessage {
  int type;
  char dest[8];
  int doReverse;
  int speed;
  int location;
  int delta;
  int tid;
  int color;
};

struct TrainVelocityMessage {
  int type;
  char dest[8];
  int doReverse;
  int speed;
  int location;
  int delta;
  int tid;
  int color;
  int velocity[15];
};

typedef enum {
  DIR_FORWARD,
  DIR_BACKWARD
} train_direction;

char *getHomeFromTrainID(int trainID);
int getColorFromTrainID(int trainID);

void copyPath(struct Path *dest, struct Path *source);
int shortestPath(int node1, int node2, track_node *track, struct Path *path, int doReverse, track_edge *badEdge);
int BFS(int node1, int node2, track_node *track, struct Path *path, int doReverse);
int adjDistance(track_node *src, track_node *dest);
int adjDirection(track_node *src, track_node *dest);
track_edge *adjEdge(track_node *src, track_node *dest);
int distanceAfterForTrackState(track_node *track, int distance, int nodeNum, int delta, int *returnDistance);
int distanceBeforeForTrackState(track_node *track, int distance, int nodeNum, int delta, int *returnDistance);
int distanceAfter(struct Path *path, int distance, int nodeNum, int *returnDistance);
int distanceBefore(struct Path *path, int distance, int nodeNum, int *returnDistance);

void periodicTask();
void delayTask();
void sensorWaitTask();
void reserveWaitTask();
void stopWaitTask();
void locationWaitTask();

void trainTask();
void removeTrainTask(int trainTid, int taskID);
void setTrainLocation(int trainTId, int location, int delta);
void waitForLocation(int trainTid, int location, int delta);
void waitForStop(int trainTid);
void setAccelerating(int trainTid);
void setDecelerating(int trainTid);
int getTrainLocation(int trainTid, int *location);
int getTrainVelocity(int trainTid);
int getTrainMaxVelocity(int trainTid);
acceleration_type getAccState(int trainTid);
void reverseTrain(int trainTid);
train_direction getTrainDirection(int trainTid);
void setTrainColor(int trainTid, int color);
int getTrainColor(int trainTid);
void setTrainSpeed(int trainTid, int speed);
int getTrainID(int trainTid);
int getTrainNum(int trainTid);

void trainGoHome(int trainTid);

int getNodeIndex(track_node *track, track_node *location);

int getNextStop(struct Path *path, int curNode);
int getNextSwitch(struct Path *path, int curNode);
int getNextSensor(struct Path *path, int curNode);
void getAllBranchMissSensors(struct Path *path, int curNode, int *sensors, int *switches);
int getNextNodeForTrackState(track_node *track, int location);
int getNextSensorForTrackState(track_node *track);
int distanceAlongPath(struct Path *path, track_node *node1, track_node *node2);

void switchWatcher();
void nodeWatcher();
void sensorWatcher();
void reserveWatcher();
void unreserveWatcher();
void trainDriver();

void trainConfiger();

#endif
