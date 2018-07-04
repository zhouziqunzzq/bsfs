#ifndef MBR_H_INCLUDED
#define MBR_H_INCLUDED

#include "BSFSParams.h"
#include <cstring>

using namespace std;

struct MBR  // sizeof(MBR) = BLOCKSIZE
{
    bool formatFlag;
    char welcomeMsg[BLOCKSIZE - 1];

    MBR()
    {
        this->formatFlag = false;
        memset(this->welcomeMsg, 0, sizeof(welcomeMsg));
    }
};

#endif // MBR_H_INCLUDED
