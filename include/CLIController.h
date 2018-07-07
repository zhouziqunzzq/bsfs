#ifndef CLICONTROLLER_H
#define CLICONTROLLER_H

#include "iNode.h"
#include "SFD.h"
#include "BSFSParams.h"

class CLIController
{
    public:
        CLIController();
        bool MakeMenu();
        bool ReadCommand();
        void DisplayMode(const char mode, char* rst);

    protected:

    private:
        int uid;
        iNode nowiNode;
        char month[12][5]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
            "Aug", "Sep", "Oct", "Nov", "Dec"};
};

#endif // CLICONTROLLER_H
