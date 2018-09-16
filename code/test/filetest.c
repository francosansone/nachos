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
#define STANDAR_INPUT 0
#define STANDAR_OUTPUT 1


int
main(int argc, char **argv)
{
    char *a = "Hello world\n";
    Write(a,29,STANDAR_OUTPUT);
    Write(a,29,STANDAR_OUTPUT);
    const char *arg1 = "Hola Mundo\n";
    for(unsigned i = 0; i < argc; i++){
      arg1 = argv[i];
      Write(arg1,4,STANDAR_OUTPUT);
    //  Write('\n',12,OUTPUT);
    }
    Create("test.txt");
    Create("probando.txt");
    char buff[128];
    if(argc > 0)
        Write(argv[argc-1],4,STANDAR_OUTPUT);
    OpenFileId o = Open("test.txt");
    Write(arg1,4,o);
    Close(o);
    o = Open("test.txt");
    Read(buff, 4,o);
    Close(o);
     OpenFileId p = Open("probando.txt");
     Write(buff, 4,p);
// //    Close(o);
     Close(p);
     Exit(0);
    // Halt();

    // Not reached.
}
