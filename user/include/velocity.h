#ifndef __VELOCITY_H__
#define __VELOCITY_H__

#include <track_node.h>

typedef enum {
  NONE,
  ACCELERATING,
  DECELERATING} acceleration_type;

struct VelocityProfile {
  int trainNum;
  struct Path *path;
  int location;
  float delta;
  float accDist;		//distance traveled so far during acceleration
  int speed;
  int velocity[15];	//current train's velocity at each speed
  int moving;
  acceleration_type accState;
  int t0;
  float v0;
  int notifier;		//periodic notifier
  int reverseNode;
  int stopNode;
  int hasReversed;
  int hasStopped;
};

void initVelocities(int trainNum, int *velocity);
float stoppingDistance(float velocity);
float startingTime(float velocity);
float startingDistance(float velocity);
float stoppingTime(float velocity);

void initProfile(struct VelocityProfile *profile, int trainNum, int speed, struct Path *path, int source, int notifier);
float currentVelocity(struct VelocityProfile *profile);
float currentPosition(struct VelocityProfile *profile);

int updateProfile(struct VelocityProfile *profile);
int waitForStop(struct VelocityProfile *profile);
int waitForDistance(struct VelocityProfile *profile, int distance);
int waitForDistanceOrStop(struct VelocityProfile *profile, int distance);

void setAccelerating(struct VelocityProfile *profile);
void setDecelerating(struct VelocityProfile *profile);
void setLocation(struct VelocityProfile *profile, int location);

#endif
