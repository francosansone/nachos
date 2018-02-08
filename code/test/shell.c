#include "syscall.h"


#define MAX_LINE_SIZE  60
#define MAX_ARG_COUNT  32
#define ARG_SEPARATOR  ' '
#define ARG_CONSTRUCTOR '"'

#define NULL  ((void *) 0)

static inline unsigned
strlen(const char *s)
{
    // TO DO: how to make sure that `s` is not `NULL`?

    unsigned i;
    if(s != NULL){
      for (i = 0; s[i] != '\0'; i++);
      return i;
    }
    else{
      return -1;
    }
}

static inline void
WritePrompt(OpenFileId output)
{
    static const char PROMPT[] = "--> ";
    Write((char *)PROMPT, sizeof PROMPT - 1, output);
}

static inline void
WriteDebug(const char *description, OpenFileId output)
{
    static const char PREFIX[] = "Debug: ";
    static const char SUFFIX[] = "\n";
    if(description != NULL){
      Write((char *)PREFIX, sizeof PREFIX - 1, output);
      Write((char *)description, strlen(description), output);
      Write((char *)SUFFIX, sizeof SUFFIX - 1, output);
    }
}

static inline void
WriteError(const char *description, OpenFileId output)
{
    // TO DO: how to make sure that `description` is not `NULL`?
    //(char *) to aviod warnings
    static const char PREFIX[] = "Error: ";
    static const char SUFFIX[] = "\n";
    if(description != NULL){
      Write((char *)PREFIX, sizeof PREFIX - 1, output);
      Write((char *)description, strlen(description), output);
      Write((char *)SUFFIX, sizeof SUFFIX - 1, output);
    }
}

static unsigned
ReadLine(char *buffer, unsigned size, OpenFileId input)
{
    // TO DO: how to make sure that `buffer` is not `NULL`?

    unsigned i;
    if(buffer != NULL){
      for (i = 0; i < MAX_LINE_SIZE; i++) {
          Read(&buffer[i], 1, input);
          // TO DO: what happens when the input ends?
          //WriteDebug("Leimos bastante", 1);
          if ((buffer[i] == '\0') || (buffer[i] == '\n')) {
              buffer[i] = '\0';
              break;
          }
      }
      WriteDebug(buffer, 1);
      return i;
    }
}

static int
PrepareArguments(char *line, char **argv, unsigned argvSize)
{
    // TO DO: how to make sure that `line` and `argv` are not `NULL`?, and
    //        for `argvSize`, what precondition should be fulfilled?
    //
    // PENDIENTE: use `bool` instead of `int` as return type; for doing this,
    //            given that we are in C and not C++, it is convenient to
    //            include `stdbool.h`.

    if((line != NULL) && (argv != NULL) && (argvSize <= MAX_ARG_COUNT)) {
      unsigned argCount;

      //argv[0] = line;

      argCount = 0;

      // Traverse the whole line and replace spaces between arguments by null
      // characters, so as to be able to treat each argument as a standalone
      // string.
      //
      // TO DO: what happens if there are two consecutive spaces?, and what
      //        about spaces at the beginning of the line?, and at the end?
      //
      // TO DO: what if the user wants to include a space as part of an
      //        argument?
      for (unsigned i = 0; line[i] != '\0' && i < MAX_LINE_SIZE; i++)
          if (line[i] == ARG_CONSTRUCTOR) {

            argv[argCount] = &line[i+1];
            i++;
              /*if (argCount == argvSize - 1)
                  // The maximum of allowed arguments is exceeded, and
                  // therefore the size of `argv` is too.  Note that 1 is
                  // decreased in order to leave space for the NULL at the end.
                  return 0;
              line[i] = '\0';
              argv[argCount] = &line[i + 1];*/
            while(line[i] != ARG_CONSTRUCTOR){
              WriteDebug("Searching arguments", 1);
              i++;
            }
            argCount++;
          }

      argv[argCount] = NULL;
    }
    WriteDebug("returning", 1);
    return 1;
}

int
main(void)
{
    const OpenFileId INPUT  = ConsoleInput;
    const OpenFileId OUTPUT = ConsoleOutput;
    char             line[MAX_LINE_SIZE];
    char            *argv[MAX_ARG_COUNT];

    for (;;) {
        WritePrompt(OUTPUT);
        const unsigned lineSize = ReadLine(line, 5, INPUT);
        if (lineSize == 0)
            continue;
        if (PrepareArguments(line, argv, MAX_ARG_COUNT) == 0) {
            WriteError("too many arguments.", OUTPUT);
            continue;
        }
        else{
          WriteDebug("out of PrepareArguments", 1);
        }
        // Comment and uncomment according to whether command line arguments

        // are given in the system call or not.
        //const SpaceId newProc = Exec(line);
        const SpaceId newProc = Exec(line, argv);

        // TO DO: check for errors when calling `Exec`; this depends on how
        //        errors are reported.

        //Join(newProc);
        // TO DO: is it necessary to check for errors after `Join` too, or
        //        can you be sure that, with the implementation of the system
        //        call handler you made, it will never give an error?; what
        //        happens if tomorrow the implementation changes and new
        //        error conditions appear?
    }

    return 0;  // Never reached.
}
