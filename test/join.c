#include "syscall.h"

int main() {
  int id, result;

  id = Exec("../test/joinfork");
  result = Join(id);

  Exit(result);
}