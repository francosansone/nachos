#include "syscall.h"
#define OUTPUT 1

//The OUTPUT could be the last argument

int main(int argc, char **argv){
  for(unsigned i = 0; i < argc; i++){
    char buff[128];               //Max size of file: 128
    OpenFileId o = Open(argv[i]); //Open the file
    for(unsigned j = 0; j < 128 && Read(buff + j, 1,o) > 0; j++)
      Write(buff + j, 1, OUTPUT);
    Close(o);
  }
  Exit(0);
}
