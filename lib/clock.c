#include <clock.h>
#include <ts7200.h>

void clockInit() {
  int *clockControl = (int *)(TIMER3_BASE + CRTL_OFFSET);
  int *clockLoad = (int *)(TIMER3_BASE + LDR_OFFSET);
  *clockLoad = 507999;
  *clockControl = ENABLE_MASK | MODE_MASK | CLKSEL_MASK;
}

int getClockTick() {
  static int *clockValue = (int *)(TIMER3_BASE + VAL_OFFSET);
  return *clockValue;
}