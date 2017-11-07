#include "synch_console.hh"
void 
fReadAvail(void  *arg){
	SynchConsole * c = (SynchConsole *) arg;
	c->ReadAvail();
}

void 
fWriteDone(void *arg){
    SynchConsole * c = (SynchConsole *) arg;
    c->WriteDone();
}

SynchConsole::SynchConsole(const char *in, const char *out)
{
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    readLock = new Lock("read lock");
    writeLock = new Lock("write lock");
    console = new Console(NULL, NULL, fReadAvail, fWriteDone, this);
}    
 
SynchConsole::~SynchConsole()
{
    delete readAvail;
    delete writeDone;
    delete readLock;
    delete writeLock;
    delete console;
}    

void
SynchConsole::ReadAvail()
{
    readAvail -> V();
}

void
SynchConsole::WriteDone()
{
    writeDone -> V();
}

char
SynchConsole::ReadChar()
{
    readLock->Acquire();
    readAvail->P();
    char ch = console->GetChar();
    readLock->Release();
    return ch;
}    

void
SynchConsole::WriteChar(char ch)
{
    writeLock->Acquire();
    console->PutChar(ch);
    writeDone -> P();
    writeLock->Release();
}

