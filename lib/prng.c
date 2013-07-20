#include <prng.h>

#include <bwio.h>

void seed(struct PRNG *generator, int newSeed) {
  generator->seed = newSeed;
}

int random(struct PRNG *generator) {
  int next = generator->seed;
  next = 36969*(next & 65535) + (next >> 16);
  generator->seed = next;
  return next;
}

int randomRange(struct PRNG *generator, int l, int h) {
  int rvalue = random(generator);

  return (rvalue % (h-l+1)) + l;
}
