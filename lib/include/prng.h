#ifndef __PRNG_H__
#define __PRNG_H__

struct PRNG {
  unsigned int seed;
};



void seed(struct PRNG *generator, unsigned int newSeed);
unsigned int random(struct PRNG *generator);
unsigned int randomRange(struct PRNG *generator, unsigned int l, unsigned int h);

#endif
