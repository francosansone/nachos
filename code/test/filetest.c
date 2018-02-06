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


int
main(int argc, char **argv)
{
    //const char *arg1 = argv[0];
    Create("test.txt");
    Create("probando.txt");
    char buff[128];
    OpenFileId o = Open("test.txt");
    Write("arg1",4,o);
    Write("Hello world\n",12,1);
    Close(o);
    o = Open("test.txt");
    Read(buff, 12,o);
    Close(o);
    OpenFileId p = Open("probando.txt");
    Write(buff, 4,p);
//    Close(o);
//    Close(p);
    Exit(0);

    // Not reached.
}
