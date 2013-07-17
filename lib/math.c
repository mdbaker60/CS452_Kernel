#include <math.h>

int pow(int b, int e) {
  int ans = 1;
  while(e > 0) {
    ans *= b;
    --e;
  }

  return ans;
}

int abs(int n) {
  return (n < 0) ? -n : n;
}
