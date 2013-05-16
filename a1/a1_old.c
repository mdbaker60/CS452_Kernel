#include <a1.h>
#include <bwio.h>

void sub() {
  bwprintf(COM2, "test\r");
}

int main() {
  *((int *)0x28) = (int)sub;

  bwprintf(COM2, "entering syscall\r");
  syscall();
  bwprintf(COM2, "returned from syscall\r");

  return 0;
}
