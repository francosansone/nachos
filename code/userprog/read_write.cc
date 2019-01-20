#include "syscall.h"
#define READ_WRITE_DEBUG 0

void
ReadStringFromUser(int userAddress, char *outString, unsigned maxByteCount)
{
    int buff;
    for (unsigned i = 0; i < maxByteCount; i++){
        DEBUG('f',"se lee la posicion %d\n", (unsigned)userAddress + i);
        if(!machine -> ReadMem((unsigned)userAddress + i, 1, &buff)){
            DEBUG('f', "fallo de pagina\n");
            // if(!machine -> ReadMem((unsigned)userAddress + i, 1, &buff))
                if(!machine -> ReadMem((unsigned)userAddress + i, 1, &buff))
                    ASSERT(false);
        }
        outString[i] = buff;
        if(outString[i] == '\0')
            return;

    }
    ASSERT(false);
}


void
ReadBufferFromUser(int userAddress, char *outBuffer, unsigned byteCount)
{
    int buff;
    for (unsigned i = 0; i < byteCount; i++){
        if(!machine -> ReadMem(userAddress + i, 1, &buff)){
            #if READ_WRITE_DEBUG
                printf("\nFallo numero 1 addr: %u\n",userAddress + i);
            #endif
            if(!machine -> ReadMem(userAddress + i, 1, &buff)){
                #if READ_WRITE_DEBUG
                    printf("\nFallo numero 2 addr: %u\n",userAddress + i);
                #endif
                ASSERT(false);
            }
        }
                  //lectura fallida
        outBuffer[i] = buff;
    }
}

void
WriteStringToUser(const char *string, int userAddress)
{
    int temp;
    for(unsigned i = 0;; i++) {
        temp = string[i];
        if(!machine -> WriteMem(userAddress + i, 1, temp))
            // if(!machine -> WriteMem(userAddress + i, 1, temp))
                if(!machine -> WriteMem(userAddress + i, 1, temp))
                    ASSERT(false);
                  //escritura fallida
        if(temp == '\0')
            return;   //escritura exitosa
    }
}


void
WriteBufferToUser(const char *buffer, int userAddress,unsigned byteCount)
{
    int temp;
    for(unsigned i = 0; i < byteCount; i++){
        temp = buffer[i];
        // if(!machine -> WriteMem(userAddress + i, 1, temp))
            if(!machine -> WriteMem(userAddress + i, 1, temp))
                if(!machine -> WriteMem(userAddress + i, 1, temp))
                    ASSERT(false);
               //escritura fallida
    }
}
