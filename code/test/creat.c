#include "syscall.h"


int main(){
    char *args[1];
    args[0] = "Hola";
    Create("Holanda.txt");
    Write("Hola Mundo", 10, 1);
    int pid = Exec("filetest", args);
    int pid1 = Exec("sort", args);
    int ret = Join(pid);
    int ret1 = Join(pid1);
    Create("Holan.txt");
//    Halt();
    Exit(0);
}    
