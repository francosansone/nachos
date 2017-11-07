#include "syscall.h"

int main(void)
{
    char buff[128];
    int n;

    while(1){
        n = Read(buff, 128, ConsoleInput);
        if (n == 0)
            break;
        Write(buff, n, ConsoleOutput);
    }
    return 0;
}
