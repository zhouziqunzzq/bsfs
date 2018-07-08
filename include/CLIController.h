#ifndef CLICONTROLLER_H
#define CLICONTROLLER_H

#include "iNode.h"
#include "SFD.h"
#include "BSFSParams.h"
#include "UserController.h"
#include "FSController.h"
#include "VHDController.h"
#include "PIController.h"

using namespace std;

class CLIController
{
    public:
        CLIController(FSController& _fsc, UserController& _uc, int uid);
        bool MakeMenu();
        bool ReadCommand();
        void DisplayMode(const char mode, char* rst);
        void GetLastSeg(char* cmd, int len, char* dirname, int &dncnt);
        bool GetProcessID(char* cmd, int len);

    protected:

    private:
        FSController& fsc;
        UserController& uc;
        VHDController& vhdc;
        PIController& pic;
        int uid;
        iNode nowiNode;
        char month[12][5]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
            "Aug", "Sep", "Oct", "Nov", "Dec"};
};

#endif // CLICONTROLLER_H
