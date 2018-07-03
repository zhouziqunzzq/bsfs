#include "FSController.h"
#include "VHDController.h"
#include "BSFSParams.h"
#include "MBR.h"
#include <cstring>
#include <iostream>

using namespace std;

FSController::FSController(VHDController& _vhdc) : vhdc(_vhdc)
{
    return;
}

bool FSController::Format()
{
    // Format MBR Block
    MBR mbr;
    mbr.formatFlag = true;
    strcpy(mbr.welcomeMsg, WELCOMEMSG);
    if (!this->vhdc.WriteBlock(MBRBLOCK, (char*)&mbr, sizeof(MBR)))
    {
        return false;
    }
    return true;
}

bool FSController::IsFormat()
{
    MBR mbr;
    return this->vhdc.ReadBlock(0, (char*)&mbr) && mbr.formatFlag;
}
