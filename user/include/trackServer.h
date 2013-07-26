#ifndef __TRACKSERVER_H__
#define __TRACKSERVER_H__

#include <track_node.h>

#define TRACKA		0
#define TRACKB		1
#define NUM_TRACKA	144
#define NUM_TRACKB	140

#define NUMTRAINS 	80
#define NUMSENSORS	80
#define NUMSWITCHES 	22
#define ANYSENSOR	80

#define TRACKPRINTCLOCK		0
#define TRACKSETSWITCH		1
#define TRACKGETSWITCH		2
#define	TRACKGETSENSOR		3
#define TRACKBLOCKSENSOR	4
#define TRACKSETSENSORS		5
#define TRACKPRINTDEBUG		6
#define TRACKREFRESHSCREEN	7
#define TRACKSETTRACK		8
#define TRACKGETTRACK		9
#define TRACKREMOVETASK		10
#define TRACKRESERVE		11
#define TRACKRESERVEBLOCK	12
#define TRACKUNRESERVE		13
#define TRACKGETNUM		14
#define TRACKRELEASEALLRESERV	15
#define TRACKPRINTRES		16

struct SensorStates {
  int stateInfo[3];
};

void getMergeEdges(track_node *node1, track_node *node2, track_edge **edge1, track_edge **edge2);

void TrackServerInit();

void printTime(int min, int sec, int tenthSec);
void setSwitchState(int switchNum, char state);
char getSwitchState(int switchNum);
void waitOnSensor(int sensorNum);
int waitOnSensors(struct SensorStates *sensors);
int waitOnAnySensor();
void setSensorData(struct SensorStates *states);
void printDebugInfo(int line, char *debugInfo);
void refreshScreen();
void setTrack(int track);
int getTrack();
int getMyTrainID();
void releaseAllReservations(int trainTid, int node1, int node2);
void printReservedTracks(int trainTid);

int getReservation(int trainTid, int node1, int node2);
int blockOnReservation(int trainTid, int node1, int node2);
void returnReservation(int node1, int node2);

void initTrack(track_node *track);

void setSensor(struct SensorStates *states, int sensorNum, int state);
int getSensor(struct SensorStates *states, int sensorNum);
int setSensorByte(struct SensorStates *states, int byteNum, int byte);
void getSensorData(struct SensorStates *states);

void removeTrackTask(int taskID);

#endif
