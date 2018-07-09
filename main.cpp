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

int main()
{
    VHDController vhdc;
    TestVHDController(vhdc);

    FSController fsc(vhdc);
    TestFSController(fsc);


    UserController uc;
    PIController pic;
    iNode nowiNode;
    if(!fsc.GetiNodeByID(ROOTDIRiNODE, &nowiNode))
        return -1;
    CLIController cli(fsc, uc, vhdc, pic, ROOT_UID, nowiNode);
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

    /*TestGLP(fsc);
    TestGetBIDByFOff(fsc);
    TestReadFileToBuf(fsc);
    TestAppendBlocksToFile(fsc);
    TestWriteFileFromBuf(fsc);
    TestCreateRootDir(fsc);
    TestCreateSubDir(fsc);
    TestParsePath(fsc);
    TestGetAbsDir(fsc);
    TestTouch(fsc);
    TestGetContentInDir(fsc);
    TestWriteFileFromBuf2(fsc);
    TestDeleteFile(fsc);
    TestGetContentInDir(fsc);
    TestDeleteDir(fsc);
    TestCopyFile(fsc);
    TestGetContentInDir(fsc);*/

    return 0;
}
