#include "syscall.h"

void ret2() {
  Yield();
  Yield();
  Exit(2);
}

int main() {
  Fork(ret2);
  Exit(1);
}