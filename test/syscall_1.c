#include "syscall.h"

void main() {
  char arr[10];
  int i, fd;
  char sum;

  Create("numbers.bin");
  fd = Open("numbers.bin");

  for (i = 0; i < 10; ++i)
    arr[i] = i + 1;
  Write(arr, sizeof(arr), fd);
  Close(fd);

  fd = Open("numbers.bin");
  for (i = 0; i < 10; ++i)
    arr[i] = 0;
  Read(arr, sizeof(arr), fd);
  Close(fd);

  sum = 0;
  for (i = 0; i < 10; ++i)
    sum += arr[i];

  Exit(sum);
}