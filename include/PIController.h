#ifndef PICONTROLLER_H
#define PICONTROLLER_H

#include "iNodeMem.h"
#include "Process.h"

class PIController
{
    public:
        PIController();
        bool CheckXlock(const bid_t bid);

    protected:

    private:
        iNodeMem ilist[MAX_iNODE_CNT];
        Process plist[MAX_PROCESS_CNT];
        pid_t pidp = 0;
};

#endif // PICONTROLLER_H
