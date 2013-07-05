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
  int delta;
  int accDist;		//distance traveled so far during acceleration
  int speed;
  int velocity[15];	//current train's velocity at each speed
  int moving;
  acceleration_type accState;
  int t0, v0;
  int notifier;		//periodic notifier
  int reverseNode;
  int stopNode;
  int hasReversed;
  int hasStopped;
};

void initVelocities(int trainNum, int *velocity);
int stoppingDistance(int velocity);
int startingTime(int velocity);
int startingDistance(int velocity);
int stoppingTime(int velocity);

void initProfile(struct VelocityProfile *profile, int trainNum, int speed, struct Path *path, int source, int notifier);
int currentVelocity(struct VelocityProfile *profile);
int currentPosition(struct VelocityProfile *profile);

int updateProfile(struct VelocityProfile *profile);
int waitForStop(struct VelocityProfile *profile);
int waitForDistance(struct VelocityProfile *profile, int distance);

void setAccelerating(struct VelocityProfile *profile);
void setDecelerating(struct VelocityProfile *profile);
void setLocation(struct VelocityProfile *profile, int location);

#endif
