#include <velocity.h>
#include <syscall.h>
#include <values.h>
#include <clockServer.h>
#include <io.h>

#include <term.h>

void initVelocities(int trainNum, int *velocity) {
  if(trainNum == 45) {
    velocity[8] = 376;
    velocity[9] = 419;
    velocity[10] = 446;
    velocity[11] = 505;
    velocity[12] = 552;
    velocity[13] = 600;
    velocity[14] = 601;
  }
}

int stoppingDistance(int velocity) {
  //int d = (14195*velocity) - 260000;//520000;
  //d /= 10000;
  int d = (45*velocity*velocity);
  d += (111163*velocity);
  d += 83066;
  d /= 100000;
  return d;
}
int startingTime(int velocity) {
  int t = (1073*velocity) - 121000;
  t /= 1000;
  return t;
}
int startingDistance(int velocity) {
  int d = (startingTime(velocity)*velocity)/200;
  return d;
}
int stoppingTime(int velocity) {
  int t = 200*stoppingDistance(velocity);
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
  profile->t0 = profile->v0 = -1;
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
      int startTime = startingTime(v1);
      int term1 = (-2)*v1;
      term1 *= timeDelta;
      term1 /= startTime;
      term1 *= timeDelta;
      term1 /= startTime;
      term1 *= timeDelta;
      term1 /= startTime;

      int term2 = 3*v1;
      term2 *= timeDelta;
      term2 /= startTime;
      term2 *= timeDelta;
      term2 /= startTime;

      return term1 + term2;
    }
  }else if(profile->accState == DECELERATING) {
    if(timeDelta >= stoppingTime(profile->v0)) {
      profile->accState = NONE;
      return 0;
    }else{
      int stopTime = stoppingTime(profile->v0);
      int term1 = 2*(profile->v0);
      term1 *= timeDelta;
      term1 /= stopTime;
      term1 *= timeDelta;
      term1 /= stopTime;
      term1 *= timeDelta;
      term1 /= stopTime;

      int term2 = (-3)*(profile->v0);
      term2 *= timeDelta;
      term2 /= stopTime;
      term2 *= timeDelta;
      term2 /= stopTime;

      return term1 + term2 + (profile->v0);
    }
  }
  return -1;
}

int currentPosition(struct VelocityProfile *profile) {
  int timeDelta = (Time() - profile->t0);
  int v1 = (profile->velocity)[profile->speed];
  if(profile->accState == NONE) {
    if(profile->moving) {
      return v1*timeDelta/100;
    }else{
      return 0;
    }
  }else if(profile->accState == ACCELERATING) {
    if(timeDelta >= startingTime(v1)) {
      profile->accState = NONE;
      int distance = startingDistance(v1);
      int timeLeft = timeDelta - startingTime(v1);
      return distance + (timeLeft*v1)/100;
    }else{
      int startTime = startingTime(v1);
      int term1 = -(startTime*v1)/200;
      term1 *= timeDelta;
      term1 /= startTime;
      term1 *= timeDelta;
      term1 /= startTime;
      term1 *= timeDelta;
      term1 /= startTime;
      term1 *= timeDelta;
      term1 /= startTime;

      int term2 = (startTime*v1)/100;
      term2 *= timeDelta;
      term2 /= startTime;
      term2 *= timeDelta;
      term2 /= startTime;
      term2 *= timeDelta;
      term2 /= startTime;

      return term1 + term2;
    }
  }else if(profile->accState == DECELERATING) {
    if(timeDelta >= stoppingTime(profile->v0)) {
      profile->accState = NONE;
      return stoppingDistance(profile->v0);
    }else{
      int stopTime = stoppingTime(profile->v0);
      int term1 = stopTime*(profile->v0)/200;
      term1 *= timeDelta;
      term1 /= stopTime;
      term1 *= timeDelta;
      term1 /= stopTime;
      term1 *= timeDelta;
      term1 /= stopTime;
      term1 *= timeDelta;
      term1 /= stopTime;
      
      int term2 = -stopTime*(profile->v0)/100;
      term2 *= timeDelta;
      term2 /= stopTime;
      term2 *= timeDelta;
      term2 /= stopTime;
      term2 *= timeDelta;
      term2 /= stopTime;

      int term3 = stopTime*(profile->v0)/100;
      term3 *= timeDelta;
      term3 /= stopTime;

      return term1 + term2 + term3;
    }
  }
  return -1;
}

int updateProfile(struct VelocityProfile *profile) {
  int src, reply;

  int reverseStopNode, reverseStopDistance;
  int stopNode, stopDistance;
  int curVelocity = currentVelocity(profile);
  int reverseStopLength = stoppingDistance(curVelocity);
  reverseStopLength = (reverseStopLength < 400) ? 0 : reverseStopLength-400;
  if(profile->reverseNode != -1) {
    reverseStopNode = distanceBefore(profile->path, reverseStopLength, profile->reverseNode, &reverseStopDistance);
  }else{
    reverseStopNode = -1;
  }
  stopNode = distanceBefore(profile->path, stoppingDistance(curVelocity), profile->stopNode, &stopDistance);

  Receive(&src, (char *)&reply, sizeof(int));
  if(src == profile->notifier) {
    Reply(src, (char *)&reply, sizeof(int));
    int offset = (profile->velocity)[profile->speed]/10;
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
	    ((profile->path)->node)[profile->location]->name, (profile->delta)/10);
    printAt(9, 1, "\e[KVelocity: %dcm/s", curVelocity);
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
