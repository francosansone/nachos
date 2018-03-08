#include "syscall.h"
#define OUTPUT 1

int main (int argc, char **argv){
  if(argc != 2){
    Write("too many arguments", 18, OUTPUT);
    Exit(-1);
  }
  char buff[128];
  Create(argv[1]);
  int src = Open(argv[0]);
  int dest = Open(argv[1]);
  for(unsigned i = 0; i < 128 && Read(buff + i, 1, src) > 0;i++)
    Write(buff + i, 1,dest);
  Close(src);
  Close(dest);
  Exit(0);
}
