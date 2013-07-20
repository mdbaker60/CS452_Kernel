#ifndef __PRNG_H__
#define __PRNG_H__

struct PRNG {
  int seed;
};



void seed(struct PRNG *generator, int newSeed);
int random(struct PRNG *generator);
int randomRange(struct PRNG *generator, int l, int h);

#endif
