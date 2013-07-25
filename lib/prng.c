#include <prng.h>

#include <bwio.h>

void seed(struct PRNG *generator, unsigned int newSeed) {
  generator->seed = newSeed;
}

unsigned int random(struct PRNG *generator) {
  unsigned int next = generator->seed;
  next = 36969*(next & 65535) + (next >> 16);
  generator->seed = next;
  return next;
}

unsigned int randomRange(struct PRNG *generator, unsigned int l, unsigned int h) {
  unsigned int rvalue = random(generator);

  return (rvalue % (h-l+1)) + l;
}
