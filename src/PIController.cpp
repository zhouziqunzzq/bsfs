#include<iostream>
#include<cstring>
#include<string>
#include "PIController.h"

PIController::PIController()
{
    return;
}

bool PIController::CreateProcess(string pname, uid_t uid, pid_t* pid)
{
    // Try to assign new pid
    pid_t newPid;
    if(!this->AssignPid(&newPid)) return false;
    // Create new process
    *pid = newPid;
    plist[newPid].pid = newPid;
    plist[newPid].pname = pname;
    plist[newPid].valid = true;
    plist[newPid].cnt = 0;
    return true;
}

bool PIController::KillProcess(const pid_t& pid)
{
    if (!plist[pid].valid) return false;
    // Release file opened by the process
    for (unsigned int i = 0; i < plist[pid].cnt; i++)
    {
        ilist[plist[pid].flist[i]].CloseByProcess(pid);
    }
    // Release the process
    plist[pid].valid = false;
    return true;
}

void PIController::CreateiNodeMem(const iNode& i)
{
    memcpy((char*)&ilist[i.bid].inode, (char*)&i, sizeof(iNode));
    ilist[i.bid].valid = true;
    ilist[i.bid].cnt = 0;
    ilist[i.bid].xlock = false;
}

bool PIController::CheckXlock(const bid_t bid)
{
    return ilist[bid].xlock;
}

bool PIController::FOpen(const pid_t pid, const iNode& i, bool xlock)
{
    // Check pid
    if (!plist[pid].valid) return false;
    // Load iNode if necessary
    if(!ilist[i.bid].valid)
        CreateiNodeMem(i);
    // Update process and inodemem
    return plist[pid].OpenFile(i.bid) && ilist[i.bid].OpenByProcess(pid, xlock);
}

bool PIController::AssignPid(pid_t* pid)
{
    pid_t p = 0;
    for (int i = 0; i < MAX_PROCESS_CNT; i++)
    {
        if (!plist[i].valid)
        {
            *pid = p;
            return false;
        }
        p++;
    }
    return false;
}
