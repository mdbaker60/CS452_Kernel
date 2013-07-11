#include <velocity.h>
#include <syscall.h>
#include <values.h>
#include <clockServer.h>
#include <io.h>

#include <term.h>

void initVelocities(int trainNum, int *velocity) {
  if(trainNum == 45) {
    velocity[0] = 0;
    velocity[1] = 0;
    velocity[2] = 0;
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

int currentVelocity(int t0, int v0, int v1, acceleration_type *accState) {
  int timeDelta = (Time() - t0);
  if(*accState == NONE) {
    return v1;
  }else if(*accState == ACCELERATING) {
    if(timeDelta >= startingTime(v1)) {
      *accState = NONE;
      return v1;
    }else{
      float startTime = startingTime(v1);
      float u = timeDelta/startTime;

      float term1 = (-2)*v1*u*u*u;
      float term2 = 3*v1*u*u;

      return term1 + term2;
    }
  }else if(*accState == DECELERATING) {
    if(timeDelta >= stoppingTime(v0)) {
      *accState = NONE;
      return 0;
    }else{
      float stopTime = stoppingTime(v0);
      float u = timeDelta/stopTime;

      float term1 = 2*v0*u*u*u;
      int term2 = (-3)*v0*u*u;

      return term1 + term2 + v0;
    }
  }
  return -1;
}

int currentPosition(int t0, int v0, int v1, acceleration_type *accState) {
  float timeDelta = Time() - t0;
  if(*accState == NONE) {
    return v1*timeDelta;
  }else if(*accState == ACCELERATING) {
    if(timeDelta >= startingTime(v1)) {
      *accState = NONE;
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
  }else if(*accState == DECELERATING) {
    if(timeDelta >= stoppingTime(v0)) {
      *accState = NONE;
      return stoppingDistance(v0);
    }else{
      float stopTime = stoppingTime(v0);
      float u = timeDelta/stopTime;

      float term1 = stopTime*v0*u*u*u*u/2;
      float term2 = -stopTime*v0*u*u*u;
      float term3 = stopTime*v0*u;

      return term1 + term2 + term3;
    }
  }
  return -1;
}
