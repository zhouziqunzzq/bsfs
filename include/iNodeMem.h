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

    bool OpenByProcess(const pid_t& pid, bool isxlock)
    {
        if (this->xlock) return false;
        if (this->cnt >= MAX_OPEN_CNT_F) return false;
        this->plist[this->cnt++] = pid;
        this->xlock = isxlock;
        if (isxlock) this->xpid = pid;
        return true;
    }

    void CloseByProcess(const pid_t& pid)
    {
        // Release XLock
        if (this->xlock && this->xpid == pid)
        {
            this->xlock = false;
            this->xpid = 0;
        }
        // Delete from plist
        for (unsigned int i = 0; i < this->cnt; i++)
        {
            if (plist[i] == pid)
            {
                for (unsigned int j = i; j < cnt - 1; j++)
                {
                    plist[j] = plist[j + 1];
                }
                this->cnt--;
                break;
            }
        }
    }
};

#endif // INODEMEM_H_INCLUDED
