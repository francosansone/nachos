#include "syscall.h"
#define OUTPUT 1
#define NULL  ((void *) 0)

int main(int argc, char **argv){
  for(unsigned i = 0; i < argc; i++){
    char *buff;               //Max size of file: 128
    OpenFileId o = Open(argv[i]); //Open the file
    Write(argv[i], 8, 1);
    Write("\n\n",2,1);
    for(unsigned j = 0; j < 128; j++){
      Read(buff + j, 1,o);
      Write(buff + j, 1, OUTPUT);
      if (buff[j] == NULL){
        char *jj = (char *)j;
        Close(o);
        break;
      }
    }
  }
  Exit(0);
}
