#include "syscall.h"


int main(){
    char *args[1];
    args[0] = "Hola";
    Create("Holanda.txt");
    Write("Hola Mundo", 13, 1);
    int pid = Exec("filetest", args);
    int ret = Join(pid);
    Create("Holan.txt");
    Halt();
    Exit(0);
}    
