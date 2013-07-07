#include <velocity.h>
#include <syscall.h>
#include <values.h>
#include <clockServer.h>
#include <io.h>

#include <term.h>

void initVelocities(int trainNum, int *velocity) {
  if(trainNum == 45) {
    velocity[3] = 107;
    velocity[4] = 160;
    velocity[5] = 216;
    velocity[6] = 270;
    velocity[7] = 334;
    velocity[8] = 376;
    velocity[9] = 419;
    velocity[10] = 446;
    velocity[11] = 505;
    velocity[12] = 552;
    velocity[13] = 600;
    velocity[14] = 601;
  }
}

float stoppingDistance(float velocity) {
  float d = (0.00045321*velocity*velocity) + (1.1116*velocity) + 0.83066;
  return d;
}
float startingTime(float velocity) {
  float t = 0.0000000002288*velocity*velocity*velocity*velocity;
  t -= 0.00000026163*velocity*velocity*velocity;
  t += 0.000081223*velocity*velocity;
  t += 0.0060737*velocity;
  t -= 0.19122;
  return t;
}
float startingDistance(float velocity) {
  float d = (startingTime(velocity)*velocity)/2;
  return d;
}
float stoppingTime(float velocity) {
  float t = 2*stoppingDistance(velocity);
  t /= velocity;
  return t;
}

void initProfile(struct VelocityProfile *profile, int trainNum, int speed, struct Path *path, int source, int notifier) {
  profile->trainNum = trainNum;
  profile->path = path;
  profile->location = source;
  profile->delta = 0;
  profile->accDist = 0;
  profile->speed = speed;
  initVelocities(trainNum, profile->velocity);
  profile->accState = NONE;
  profile->notifier = notifier;
  profile->reverseNode = -1;
  profile->stopNode = (path->numNodes)-1;
  profile->hasReversed = false;
  profile->hasStopped = false;
}

float currentVelocity(struct VelocityProfile *profile) {
  float timeDelta = (Time() - profile->t0);
  float v1 = (profile->velocity)[profile->speed];
  if(profile->accState == NONE) {
    if(profile->moving) {
      return v1;
    }else{
      return 0;
    }
  }else if(profile->accState == ACCELERATING) {
    if(timeDelta >= startingTime(v1)*100) {
      profile->accState = NONE;
      return v1;
    }else{
      float startTime = startingTime(v1);
      float u = (timeDelta/startTime)/100;

      float term1 = (-2)*v1*u*u*u;
      float term2 = 3*v1*u*u;

      return term1 + term2;
    }
  }else if(profile->accState == DECELERATING) {
    if(timeDelta >= stoppingTime(profile->v0)*100) {
      profile->accState = NONE;
      return 0;
    }else{
      float stopTime = stoppingTime(profile->v0);
      float u = (timeDelta/stopTime)/100;

      float term1 = 2*(profile->v0)*u*u*u;
      int term2 = (-3)*(profile->v0)*u*u;

      return term1 + term2 + (profile->v0);
    }
  }
  return -1;
}

float currentPosition(struct VelocityProfile *profile) {
  float timeDelta = (Time() - profile->t0);
  float v1 = (profile->velocity)[profile->speed];
  if(profile->accState == NONE) {
    if(profile->moving) {
      return v1*timeDelta;
    }else{
      return profile->accDist;
    }
  }else if(profile->accState == ACCELERATING) {
    if(timeDelta >= startingTime(v1)*100) {
      profile->accState = NONE;
      float distance = startingDistance(v1);
      float timeLeft = timeDelta - 100*startingTime(v1);
      return distance + (timeLeft*v1)/100;
    }else{
      float startTime = startingTime(v1);
      float u = (timeDelta/startTime)/100;

      float term1 = -(startTime*v1*u*u*u*u)/2;
      float term2 = (startTime*v1)*u*u*u;

      return term1 + term2;
    }
  }else if(profile->accState == DECELERATING) {
    if(timeDelta >= stoppingTime(profile->v0)*100) {
      profile->accState = NONE;
      return stoppingDistance(profile->v0);
    }else{
      float stopTime = stoppingTime(profile->v0);
      float u = (timeDelta/stopTime)/100;

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
  reverseStopLength = (reverseStopLength < 300) ? 0 : reverseStopLength-300;
  if(profile->reverseNode != -1) {
    reverseStopNode = distanceBefore(profile->path, reverseStopLength, profile->reverseNode, &reverseStopDistance);
  }else{
    reverseStopNode = -1;
  }
  stopNode = distanceBefore(profile->path, stopLength, profile->stopNode, &stopDistance);

  Receive(&src, (char *)&reply, sizeof(int));
  if(src == profile->notifier) {
    Reply(src, (char *)&reply, sizeof(int));
    float offset = (profile->velocity)[profile->speed]/10;	//update every 10th second
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
	    ((profile->path)->node)[profile->location]->name, (int)(profile->delta)/10);
    printAt(9, 1, "\e[KVelocity: %dmm/s", (int)currentVelocity(profile));
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
  profile->delta = 0;
  profile->hasReversed = false;
}
