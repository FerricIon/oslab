#include "syscall.h"

int shared;

void Forked() {
  int i;
  for (i = 0; i < 5; ++i) {
    shared++;
    Yield();
  }
  Exit(shared);
}

int main() {
  shared = 0;
  Fork(Forked);
  Forked();
}