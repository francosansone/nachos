#include "syscall.h"


int main(){
    Create("Holanda.txt");
    Write("Hola Mundo", 13, 1);
    int pid = Exec("filetest");
    int ret = Join(pid);
    Create("Holan.txt");
    Halt();
    Exit(0);
}    
