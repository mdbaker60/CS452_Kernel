#include <velocity.h>
#include <syscall.h>
#include <values.h>
#include <clockServer.h>
#include <io.h>

#include <term.h>

void initVelocities(int trainNum, int *velocity) {
  if(trainNum == 45) {
    velocity[3] = 1070;
    velocity[4] = 1600;
    velocity[5] = 2160;
    velocity[6] = 2700;
    velocity[7] = 3340;
    velocity[8] = 3760;
    velocity[9] = 4190;
    velocity[10] = 4460;
    velocity[11] = 5050;
    velocity[12] = 5520;
    velocity[13] = 6000;
    velocity[14] = 6010;
  }
}

int stoppingDistance(int velocity) {
  float d = (0.0042371*velocity*velocity) + (111.9108*velocity) + 4629.3127;
  return (int)d;
}
int startingTime(int velocity) {
  float t = (0.000000000002288*velocity*velocity)*velocity*velocity;
  t -= (0.0000000261626*velocity*velocity)*velocity;
  t += 0.0000812228*velocity*velocity;
  t += 0.0607366*velocity;
  t -= 0.191221;
  return (int)t;
}
int startingDistance(int velocity) {
  float d = (startingTime(velocity)*velocity)/2;
  return (int)d;
}
int stoppingTime(int velocity) {
  float t = 2*stoppingDistance(velocity);
  t /= velocity;
  return (int)t;
}

void initProfile(struct VelocityProfile *profile, int trainNum, int speed, struct Path *path, int source, int notifier, int *velocity) {
  profile->trainNum = trainNum;
  profile->path = path;
  profile->location = source;
  profile->displayLocation = source;
  profile->delta = 0;
  profile->accDist = 0;
  profile->speed = speed;
  profile->velocity = velocity;
  profile->accState = NONE;
  profile->notifier = notifier;
  profile->reverseNode = -1;
  profile->stopNode = (path->numNodes)-1;
  profile->hasReversed = false;
  profile->hasStopped = false;
}

int currentVelocity(struct VelocityProfile *profile) {
  int timeDelta = (Time() - profile->t0);
  int v1 = (profile->velocity)[profile->speed];
  if(profile->accState == NONE) {
    if(profile->moving) {
      return v1;
    }else{
      return 0;
    }
  }else if(profile->accState == ACCELERATING) {
    if(timeDelta >= startingTime(v1)) {
      profile->accState = NONE;
      return v1;
    }else{
      float startTime = startingTime(v1);
      float u = timeDelta/startTime;

      float term1 = (-2)*v1*u*u*u;
      float term2 = 3*v1*u*u;

      return term1 + term2;
    }
  }else if(profile->accState == DECELERATING) {
    if(timeDelta >= stoppingTime(profile->v0)) {
      profile->accState = NONE;
      return 0;
    }else{
      float stopTime = stoppingTime(profile->v0);
      float u = timeDelta/stopTime;

      float term1 = 2*(profile->v0)*u*u*u;
      int term2 = (-3)*(profile->v0)*u*u;

      return term1 + term2 + (profile->v0);
    }
  }
  return -1;
}

int currentPosition(struct VelocityProfile *profile) {
  float timeDelta = Time() - profile->t0;
  float v1 = (profile->velocity)[profile->speed];
  if(profile->accState == NONE) {
    if(profile->moving) {
      return v1*timeDelta;
    }else{
      return profile->accDist;
    }
  }else if(profile->accState == ACCELERATING) {
    if(timeDelta >= startingTime(v1)) {
      profile->accState = NONE;
      float distance = startingDistance(v1);
      float timeLeft = timeDelta - startingTime(v1);
      return distance + (timeLeft*v1);
    }else{
      float startTime = startingTime(v1);
      float u = timeDelta/startTime;

      float term1 = -(startTime*v1*u*u*u*u)/2;
      float term2 = (startTime*v1)*u*u*u;

      return term1 + term2;
    }
  }else if(profile->accState == DECELERATING) {
    if(timeDelta >= stoppingTime(profile->v0)) {
      profile->accState = NONE;
      return stoppingDistance(profile->v0);
    }else{
      float stopTime = stoppingTime(profile->v0);
      float u = timeDelta/stopTime;

      float term1 = stopTime*(profile->v0)*u*u*u*u/2;
      float term2 = -stopTime*(profile->v0)*u*u*u;
      float term3 = stopTime*(profile->v0)*u;

      return term1 + term2 + term3;
    }
  }
  return -1;
}

int updateProfile(struct VelocityProfile *profile) {
  int src, reply;

  int reverseStopNode, reverseStopDistance;
  int stopNode, stopDistance;
  float curVelocity = currentVelocity(profile);
  int reverseStopLength = stoppingDistance(curVelocity);
  int stopLength = reverseStopLength;
  reverseStopLength = (reverseStopLength < 300000) ? 0 : reverseStopLength-300000;
  if(profile->reverseNode != -1) {
    reverseStopNode = distanceBefore(profile->path, reverseStopLength, profile->reverseNode, &reverseStopDistance);
  }else{
    reverseStopNode = -1;
  }
  stopNode = distanceBefore(profile->path, stopLength, profile->stopNode, &stopDistance);

  Receive(&src, (char *)&reply, sizeof(int));
  if(src == profile->notifier) {
    Reply(src, (char *)&reply, sizeof(int));
    float offset = (profile->velocity)[profile->speed]*10;	//update every 10 ticks
    if(!(profile->moving)) {
      offset = 0;
    }
    if(profile->accState != NONE) {
      offset = currentPosition(profile);
      offset -= profile->accDist;
      profile->accDist += offset;
      if(profile->accState == NONE) profile->accDist = 0;
    }
    profile->delta += offset;

    if(!(profile->hasReversed) && reverseStopNode == profile->location && profile->delta >= reverseStopDistance) {
      Putc2(1, (char)0, (char)profile->trainNum);
      setDecelerating(profile);
      profile->hasReversed = true;
    }else if(!(profile->hasStopped) && stopNode == profile->location && profile->delta >= stopDistance) {
      Putc2(1, (char)0, (char)profile->trainNum);
      setDecelerating(profile);
      profile->hasStopped = true;
    }

    printAt(8, 1, "\e[K%s + %dcm", 
	    ((profile->path)->node)[profile->displayLocation]->name, (int)(profile->delta)/10000);
    printAt(9, 1, "\e[KVelocity: %dum/tick", (int)currentVelocity(profile));
  }
  return src;
}

int waitForStop(struct VelocityProfile *profile) {
  int src;
  while(currentVelocity(profile) > 0) {
    src = updateProfile(profile);
    if(src != profile->notifier) {
      return src;
    }
  }
  return profile->notifier;
}

int waitForDistance(struct VelocityProfile *profile, int distance) {
  int src;
  while(profile->delta < distance) {
    src = updateProfile(profile);
    if(src != profile->notifier) {
      return src;
    }
  }
  return profile->notifier;
}

int waitForDistanceOrStop(struct VelocityProfile *profile, int distance) {
  int src;
  while(profile->delta < distance && currentVelocity(profile) > 0) {
    src = updateProfile(profile);
    if(src != profile->notifier) {
      return src;
    }
  }
  return profile->notifier;
}

void setAccelerating(struct VelocityProfile *profile) {
  profile->v0 = 0;
  profile->accState = ACCELERATING;
  profile->t0 = Time();
  profile->moving = true;
  profile->accDist = 0;
}

void setDecelerating(struct VelocityProfile *profile) {
  profile->v0 = currentVelocity(profile);
  profile->accState = DECELERATING;
  profile->t0 = Time();
  profile->moving = false;
  profile->accDist = 0;
}

void setLocation(struct VelocityProfile *profile, int location) {
  profile->location = location;
  profile->displayLocation = location;
  profile->delta = 0;
  profile->hasReversed = false;
}
