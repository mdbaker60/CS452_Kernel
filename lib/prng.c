#include <prng.h>

static int next = 0;

void seed(int newSeed) {
  next = newSeed;
}

int random() {
  next = 36969*(next & 65535) + (next >> 16);
  return next;
}
