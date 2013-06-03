#include <prng.h>

#include <bwio.h>

static int next;

void seed(int newSeed) {
  next = newSeed;
}

int random() {
  next = 36969*(next & 65535) + (next >> 16);
  return next;
}
