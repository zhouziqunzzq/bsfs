#ifndef SUPERBLOCK_H_INCLUDED
#define SUPERBLOCK_H_INCLUDED

#include "BSFSParams.h"
#include <cstring>

using namespace std;

struct SuperBlock   // sizeof(SuperBlock) = BLOCKSIZE
{
    unsigned int cnt;    // Free blocks in the group, also used for stack top pointer
    bid_t freeStack[GROUPSIZE];   // Free blocks stack

    SuperBlock()
    {
        this->cnt = 0;
        memset(this->freeStack, 0, sizeof(this->freeStack));
    }
};

#endif // SUPERBLOCK_H_INCLUDED
