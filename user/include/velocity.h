#ifndef __VELOCITY_H__
#define __VELOCITY_H__

#include <track_node.h>

typedef enum {
  NONE,
  ACCELERATING,
  DECELERATING} acceleration_type;

void initVelocities(int trainNum, int *velocity);
int stoppingDistance(int trainNum, int velocity);
int startingTime(int trainNum, int velocity);
int startingDistance(int trainNum, int velocity);
int stoppingTime(int trainNum, int velocity);

int currentPosition(int t0, int v0, int v1, acceleration_type *accState, int trainNum);
int currentVelocity(int t0, int v0, int v1, acceleration_type *accState, int trainNum);

#endif
