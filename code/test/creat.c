#include "syscall.h"


int main(){
    char *args[1];
    args[0] = "Hola";
    Create("Holanda.txt");
    Write("Hola Mundo", 10, 1);
//    int pid1 = Exec("sort", args);
    int pid2 = Exec("matmult", args);
    int pid = Exec("filetest", args);
    int ret = Join(pid);
//    int ret1 = Join(pid1);
    int ret2 = Join(pid2);
    Create("Holan.txt");
//    Halt();
    Exit(0);
}    
