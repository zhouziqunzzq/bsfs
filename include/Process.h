#ifndef PROCESS_H_INCLUDED
#define PROCESS_H_INCLUDED

#include<iostream>
#include<string>
#include "BSFSParams.h"

using namespace std;

struct Process
{
    pid_t pid;
    string pname;
    uid_t uid;
    unsigned int cnt;
    bid_t flist[MAX_OPEN_CNT_P];
    bool valid;

    Process()
    {
        pid = 0;
        uid = 0;
        cnt = 0;
        memset(flist, 0, sizeof(flist));
        valid = false;
    }

    bool OpenFile(const bid_t& bid)
    {
        if (this->cnt >= MAX_OPEN_CNT_P) return false;
        this->flist[this->cnt++] = bid;
        return true;
    }

    void CloseFile(const bid_t& bid)
    {
        // Delete from flist
        for (unsigned int i = 0; i < this->cnt; i++)
        {
            if (flist[i] == bid)
            {
                for (unsigned int j = i; j < cnt - 1; j++)
                {
                    flist[j] = flist[j + 1];
                }
                this->cnt--;
                break;
            }
        }
    }
};

#endif // PROCESS_H_INCLUDED
