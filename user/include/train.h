#ifndef __TRAIN_H__
#define __TRAIN_H__

#include <track_node.h>

#define TRACKA		0
#define TRACKB		1

#define NUMTRAINS 	80
#define NUMSENSORS	80
#define NUMSWITCHES 	22
#define ANYSENSOR	80

#define TRAINPRINTCLOCK		0
#define TRAINSETSWITCH		1
#define TRAINGETSWITCH		2
#define	TRAINGETSENSOR		3
#define TRAINBLOCKSENSOR	4
#define TRAINSETSENSORS		5
#define TRAINPRINTDEBUG		6
#define TRAINREFRESHSCREEN	7
#define TRAINSETTRACK		8
#define TRAINGETTRACK		9
#define TRAINRESETBUFFER	10

struct SensorStates {
  int stateInfo[3];
};

void TrainInit();

void printTime(int min, int sec, int tenthSec);
void resetSensorBuffer();
void setSwitchState(int switchNum, char state);
char getSwitchState(int switchNum);
void waitOnSensor(int sensorNum);
int waitOnAnySensor();
void setSensorData(struct SensorStates *states);
void printDebugInfo(int line, char *debugInfo);
void refreshScreen();
void setTrack(int track);
int getTrack();

void initTrack(track_node *track);

void setSensor(struct SensorStates *states, int sensorNum, int state);
int getSensor(struct SensorStates *states, int sensorNum);
int setSensorByte(struct SensorStates *states, int byteNum, int byte);
void getSensorData(struct SensorStates *states);

#endif
