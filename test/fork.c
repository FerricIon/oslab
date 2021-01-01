#include "syscall.h"

int main() {
  int i;

  Exec("../test/exec");
  Yield();
  Yield();

  Exit(0);
}