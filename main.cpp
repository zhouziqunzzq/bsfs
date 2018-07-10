#include <iostream>
#include <cstring>
#include <ctime>
#include "test.h"
#include "BSFSParams.h"
#include "VHDController.h"
#include "FSController.h"
#include "UserController.h"
#include "PIController.h"
#include "CLIController.h"

using namespace std;

void InitVHD(VHDController& vhdc)
{
    if (!vhdc.Exists())
    {
        cout << "Initializing VHD file " << vhdc.Getfilename() << "...";
        if (vhdc.Create())
            cout << "Done" << endl;
        else
        {
            cout << "Error" << endl;
            exit(-1);
        }

    }
    cout << "Loading VHD file " << vhdc.Getfilename() << "...";
    if (vhdc.Load())
        cout << "Done" << endl;
    else
    {
        cout << "Error" << endl;
        exit(-1);
    }
}

void InitFS(FSController& fsc)
{
    if (!fsc.IsFormat())
    {
        cout << "Formatting Filesystem...";
        if (fsc.Format())
            cout << "Done" << endl;
        else
        {
            cout << "Error" << endl;
            exit(-2);
        }
    }
}

void InitUser(UserController& uc)
{
    cout << "Loading User...";
    if (uc.InitLoadUserFile())
        cout << "Done" << endl;
    else
    {
        cout << "Error" << endl;
        exit(-3);
    }
}

int main()
{
    VHDController vhdc;
    InitVHD(vhdc);

    FSController fsc(vhdc);
    InitFS(fsc);

    UserController uc(fsc);
    InitUser(uc);

    PIController pic;
    iNode rootiNode;
    if(!fsc.GetiNodeByID(ROOTDIRiNODE, &rootiNode))
        return -1;
    CLIController cli(fsc, uc, vhdc, pic, ROOT_UID, rootiNode);
    // Login
    while (!cli.Login());
    // Main loop
    bool flag = false;
    while(true)
    {
        if(!cli.MakeMenu())
        {
            cout << "Error!" << endl;
            break;
        }
        cli.ReadCommand(flag);
        if(flag) break;
    }

    return 0;
}
