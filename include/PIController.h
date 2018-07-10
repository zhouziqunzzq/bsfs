#ifndef PICONTROLLER_H
#define PICONTROLLER_H

#include "iNodeMem.h"
#include "Process.h"
#include "FSController.h"
#include "BSFSParams.h"

class PIController
{
    public:
        PIController(FSController& _fsc);
        bool CheckXlock(const bid_t bid);
        bool CreateProcess(string pname, uid_t uid, pid_t* pid);
        bool KillProcess(const pid_t& pid);
        void CreateiNodeMem(const iNode& i);
        bool FOpen(const pid_t pid, const iNode& i, bool xlock);
        bool FClose(const pid_t pid, const iNode& i);
        // Print process list. For each process print a list of files
        // opened by the process
        void PrintStatus();

    protected:

    private:
        FSController& fsc;
        iNodeMem ilist[MAX_INODE_CNT];
        Process plist[MAX_PROCESS_CNT];
        bool AssignPid(pid_t* pid);
};

#endif // PICONTROLLER_H
