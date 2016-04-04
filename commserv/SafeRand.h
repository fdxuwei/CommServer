#ifndef _SAFE_RAND_H_
#define _SAFE_RAND_H_

#include <assert.h>
#include <stdlib.h>

extern __thread bool randInit;
extern __thread unsigned int randSeed;
void initRand();

inline int safeRand()
{
    if(!randInit)
    {
        initRand();
    }
    return rand_r(&randSeed);
}

#endif
