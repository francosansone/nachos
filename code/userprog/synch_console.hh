#include "machine/console.hh"
#include "threads/synch.hh"


/// La siguiente clase define una abstraccion de una consola



class SynchConsole {
public:
    
    // Inicializa la consola
    SynchConsole(const char *in, const char *out);

    // Libera la memoria guardada por SynchConsole
    ~SynchConsole();

    // Leer/escribir en la consola
    
    char ReadChar();
    void WriteChar(char data);

    // Llamada de la consola para avisar que ha terminado una operacion
    void ReadAvail();
    void WriteDone();

private:

    Console *console;
    Semaphore *readAvail;
    Semaphore *writeDone;
    Lock *readLock;
    Lock *writeLock;
};
