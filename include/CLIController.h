#ifndef CLICONTROLLER_H
#define CLICONTROLLER_H

#include "iNode.h"
#include "SFD.h"
#include "BSFSParams.h"
#include "UserController.h"
#include "FSController.h"

using namespace std;

class CLIController
{
    public:
        CLIController(FSController& _fsc, UserController& _uc, int uid);
        bool MakeMenu();
        bool ReadCommand();
        void DisplayMode(const char mode, char* rst);

    protected:

    private:
        FSController& fsc;
        UserController& uc;
        int uid;
        iNode nowiNode;
        char month[12][5]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
            "Aug", "Sep", "Oct", "Nov", "Dec"};
};

#endif // CLICONTROLLER_H
