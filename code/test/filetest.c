/// Simple program to test whether running a user program works.
///
/// Just do a “syscall” that shuts down the OS.
///
/// NOTE: for some reason, user programs with global data structures
/// sometimes have not worked in the Nachos environment.  So be careful out
/// there!  One option is to allocate data structures as automatics within a
/// procedure, but if you do this, you have to be careful to allocate a big
/// enough stack to hold the automatics!


#include "syscall.h"
#define INPUT 0
#define OUTPUT 1


int
main(int argc, char **argv)
{
    char *a = "Hello world";
    Write(a,29,OUTPUT);
    Write(a,29,OUTPUT);
    const char *arg1;
    for(unsigned i = 0; i < argc; i++){
      arg1 = argv[i];
      Write(arg1,4,OUTPUT);
    //  Write('\n',12,OUTPUT);
  }
    Create("test.txt");
    Create("probando.txt");
    char buff[128];
    Write(argv[argc-1],4,OUTPUT);
    OpenFileId o = Open("test.txt");
    Write(arg1,4,o);
    Close(o);
    o = Open("test.txt");
    Read(buff, 4,o);
    Close(o);
    OpenFileId p = Open("probando.txt");
    Write(buff, 4,p);
//    Close(o);
    Close(p);
    Exit(0);

    // Not reached.
}
