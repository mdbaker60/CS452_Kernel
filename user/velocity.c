#include <velocity.h>
#include <syscall.h>
#include <values.h>
#include <clockServer.h>
#include <io.h>

#include <term.h>

void initVelocities(int trainNum, int *velocity) {
  if(trainNum == 45 || trainNum == 48) {
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
  }else if(trainNum == 49) {
    velocity[0] = 0;
    velocity[1] = 0;
    velocity[2] = 0;
    velocity[3] = 1190;
    velocity[4] = 1710;
    velocity[5] = 2220;
    velocity[6] = 2730;
    velocity[7] = 3360;
    velocity[8] = 3820;
    velocity[9] = 4420;
    velocity[10] = 4990;
    velocity[11] = 5560;
    velocity[12] = 6140;
    velocity[13] = 6490;
    velocity[14] = 6530;
  }else if(trainNum == 50) {
    velocity[0] = 0;
    velocity[1] = 0;
    velocity[2] = 0;
    velocity[3] = 1120;
    velocity[4] = 1630;
    velocity[5] = 2220;
    velocity[6] = 2730;
    velocity[7] = 3360;
    velocity[8] = 3820;
    velocity[9] = 4420;
    velocity[10] = 4990;
    velocity[11] = 5560;
    velocity[12] = 6140;
    velocity[13] = 6490;
    velocity[14] = 6530;
  }
}

int stoppingDistance(int trainNum, int velocity) {
  if(trainNum == 45 || trainNum == 48) {
    float d = (0.0042371*velocity*velocity) + (111.9108*velocity) + 4629.3127;
    return (int)d;
  }else if(trainNum == 49 || trainNum == 50) {
    float d = (-0.000159206*velocity*velocity) + (129.49775*velocity) + 62.347;
    return (int)d;
  }
}
int startingTime(int trainNum, int velocity) {
  float t = (0.000000000002288*velocity*velocity);
  t *= velocity*velocity;
  float temp = (0.0000000261626*velocity*velocity);
  temp *= velocity;
  t -= temp;
  t += 0.0000812228*velocity*velocity;
  t += 0.0607366*velocity;
  t -= 0.191221;
  return (int)t;
}
int startingDistance(int trainNum, int velocity) {
  float d = (startingTime(trainNum, velocity)*velocity)/2;
  return (int)d;
}
int stoppingTime(int trainNum, int velocity) {
  float t = 2*stoppingDistance(trainNum, velocity);
  t /= velocity;
  return (int)t;
}

int currentVelocity(int t0, int v0, int v1, acceleration_type *accState, int trainNum) {
  int timeDelta = (Time() - t0);
  if(*accState == NONE) {
    return v1;
  }else if(*accState == ACCELERATING) {
    if(timeDelta >= startingTime(trainNum, v1)) {
      *accState = NONE;
      return v1;
    }else{
      float startTime = startingTime(trainNum, v1);
      float u = timeDelta/startTime;

      float term1 = (-2)*v1*u*u*u;
      float term2 = 3*v1*u*u;

      return term1 + term2;
    }
  }else if(*accState == DECELERATING) {
    if(timeDelta >= stoppingTime(trainNum, v0)) {
      *accState = NONE;
      return 0;
    }else{
      float stopTime = stoppingTime(trainNum, v0);
      float u = timeDelta/stopTime;

      float term1 = 2*v0*u*u*u;
      int term2 = (-3)*v0*u*u;

      return term1 + term2 + v0;
    }
  }
  return -1;
}

int currentPosition(int t0, int v0, int v1, acceleration_type *accState, int trainNum) {
  float timeDelta = Time() - t0;
  if(*accState == NONE) {
    return v1*timeDelta;
  }else if(*accState == ACCELERATING) {
    if(timeDelta >= startingTime(trainNum, v1)) {
      *accState = NONE;
      float distance = startingDistance(trainNum, v1);
      float timeLeft = timeDelta - startingTime(trainNum, v1);
      return distance + (timeLeft*v1);
    }else{
      float startTime = startingTime(trainNum, v1);
      float u = timeDelta/startTime;

      float term1 = -(startTime*v1*u*u*u*u)/2;
      float term2 = (startTime*v1)*u*u*u;

      return term1 + term2;
    }
  }else if(*accState == DECELERATING) {
    if(timeDelta >= stoppingTime(trainNum, v0)) {
      *accState = NONE;
      return stoppingDistance(trainNum, v0);
    }else{
      float stopTime = stoppingTime(trainNum, v0);
      float u = timeDelta/stopTime;

      float term1 = stopTime*v0*u*u*u*u/2;
      float term2 = -stopTime*v0*u*u*u;
      float term3 = stopTime*v0*u;

      return term1 + term2 + term3;
    }
  }
  return -1;
}
