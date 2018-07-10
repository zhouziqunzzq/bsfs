#include<iostream>
#include<cstring>
#include<string>
#include "PIController.h"

PIController::PIController(FSController& _fsc) : fsc(_fsc)
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

bool PIController::FClose(const pid_t pid, const iNode& i)
{
    // Check pid
    if (!plist[pid].valid) return false;
    // Check iNode
    if(!ilist[i.bid].valid) return false;
    // Update process and inodemem
    plist[pid].CloseFile(i.bid);
    ilist[i.bid].CloseByProcess(pid);
    return true;
}

bool PIController::AssignPid(pid_t* pid)
{
    pid_t p = 0;
    for (int i = 0; i < MAX_PROCESS_CNT; i++)
    {
        if (!plist[i].valid)
        {
            *pid = p;
            return true;
        }
        p++;
    }
    return false;
}

void PIController::PrintStatus()
{
    bool hasProcess = false;
    char path[MAX_CMD_LEN];
    for (int i = 0; i < MAX_PROCESS_CNT; i++)
    {
        if (plist[i].valid)
        {
            hasProcess = true;
            const Process& p = plist[i];
            cout << "PID:\t" << p.pid << endl;
            cout << "PName:\t" << p.pname << endl;
            cout << "UID:\t" << p.uid << endl;
            cout << "FileList:" << endl;
            for (unsigned int j = 0; j < p.cnt; j++)
            {
                cout << j << ". ";
                memset(path, 0, MAX_CMD_LEN);
                fsc.GetAbsDir(ilist[i].inode, path);
                cout << path << endl;
            }
            cout << endl;
        }
    }
    if (!hasProcess)
        cout << "No active process" << endl;
}
