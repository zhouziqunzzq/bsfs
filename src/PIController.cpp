#include<iostream>
#include<cstring>
#include<string>
#include "PIController.h"

PIController::PIController()
{
    //ctor
}

bool PIController::CreateProcess(string pname, pid_t* pid)
{
    if(pidp >= MAX_PROCESS_CNT) return false;

    plist[pidp].pid = pidp;
    *pid = pidp;
    plist[pidp].pname = pname;
    plist[pidp].valid = true;
    plist[pidp].cnt = 0;
    pidp++;
    return true;
}

bool PIController::CreateiNodeMem(const iNode& i)
{
    if(ilist[i.bid].valid) return false;

    memcpy((char*)&ilist[i.bid].inode, (char*)&i, sizeof(iNode));
    ilist[i.bid].valid = true;
    ilist[i.bid].cnt = 0;
    ilist[i.bid].xlock = false;
    return true;
}

bool PIController::CheckXlock(const bid_t bid)
{
    return ilist[bid].xlock;
}

bool PIController::FOpen(const pid_t pid, const iNode& i, bool xlock)
{
    if(!CreateiNodeMem(i)) return false;
    if(ilist[i.bid].cnt >= MAX_OPEN_CNT_F) return false;
    if(xlock && ilist[i.bid].xlock) return false;

    ilist[i.bid].plist[ilist[i.bid].cnt++] = pid;
    ilist[i.bid].xlock = xlock;
    if(xlock) ilist[i.bid].xpid = pid;

    plist[pid].flist[plist[pid].cnt++] = i.bid;
    return true;
}
