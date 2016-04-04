#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "SafeRand.h"
 

__thread bool randInit = false;
__thread unsigned int randSeed;

void initRand()
{
    randSeed = static_cast<unsigned int>(time(NULL)+syscall(SYS_gettid));
    randInit = true;
}
