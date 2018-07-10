#ifndef CLICONTROLLER_H
#define CLICONTROLLER_H

#include "iNode.h"
#include "SFD.h"
#include "BSFSParams.h"
#include "UserController.h"
#include "FSController.h"
#include "VHDController.h"
#include "PIController.h"
#include "Line.h"

using namespace std;

class CLIController
{
    public:
        CLIController(FSController& _fsc, UserController& _uc, VHDController& _vhdc,
                        PIController& _pic, int uid, const iNode& nowiNode);
        bool MakeMenu();
        bool ReadCommand(bool &exitFlag);
        void DisplayMode(const char mode, char* rst);
        void GetLastSeg(char* cmd, int len, char* dirname, int &dncnt);
        bool GetProcessID(char* cmd, int len, pid_t& pid);
        bool Login();

        void VimInit(int row);
        void VimRetkey(int ch, int& ret);
        void VimDir();
        void VimBac();
        void VimLet();
        bool VimEditor(bool& saveFlag, char* fname);

    protected:

    private:
        FSController& fsc;
        UserController& uc;
        VHDController& vhdc;
        PIController& pic;
        uid_t uid;
        iNode nowiNode;
        char month[12][5]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
            "Aug", "Sep", "Oct", "Nov", "Dec"};
        // vim params
        char msg[VIM_MAX_X][VIM_MAX_Y];
        int x, y;
        int ch;
        int xmin, xmax;
        Line line[VIM_MAX_X];
};

#endif // CLICONTROLLER_H
