#ifndef INODEMEM_H_INCLUDED
#define INODEMEM_H_INCLUDED

#include<iostream>
#include "iNode.h"
#include "BSFSParams.h"

struct iNodeMem
{
    iNode inode;
    unsigned int cnt;
    pid_t plist[MAX_OPEN_CNT_F];
    bool valid;
    bool xlock;
    pid_t xpid;

    iNodeMem()
    {
        valid = false;
        cnt = 0;
        xlock = false;
    }

    iNodeMem(const iNode& i)
    {
        valid = true;
        memcpy((char*)&inode, (char*)&i, sizeof(iNode));
        cnt = 0;
        xlock = false;
    }
};

#endif // INODEMEM_H_INCLUDED
