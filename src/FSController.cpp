#include "FSController.h"
#include "VHDController.h"
#include "BSFSParams.h"
#include "MBR.h"
#include "iNode.h"
#include <cstring>
#include <iostream>

using namespace std;

FSController::FSController(VHDController& _vhdc) : vhdc(_vhdc),
    fbc(_vhdc, SUPERBLOCK, DATABLOCK_MIN, DATABLOCK_MAX),
    ifbc(_vhdc, iSUPERBLOCK, iNODEBLOCK_MIN, iNODEBLOCK_MAX)
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

    // Format Super Block and construct GPL
    for(int i = DATABLOCK_MIN; i <= DATABLOCK_MAX; i++)
        if(!fbc.Recycle(i)) return false;
    if(!fbc.SaveSuperBlock()) return false;

    // Fomat iNode Super Block and construct GPL
    for(int i = iNODEBLOCK_MIN + 1; i <= iNODEBLOCK_MAX; i++)
        if(!ifbc.Recycle(i)) return false;
    if(!ifbc.SaveSuperBlock()) return false;

    return true;
}

bool FSController::IsFormat()
{
    MBR mbr;
    return this->vhdc.ReadBlock(0, (char*)&mbr) && mbr.formatFlag;
}

bool FSController::GetBIDByFOff(const iNode& cur, int foff, int* rst)
{
    int startBlock = foff / BLOCKSIZE;
    if (startBlock <= INODE_DIRECT_MAX)
        *rst = cur.data[startBlock];
    else if (startBlock <= (INODE_DIRECT_MAX + BLOCKSIZE / sizeof(int)))
    {
        char buf[BLOCKSIZE];
        if(!vhdc.ReadBlock(cur.data[INODE_1INDIR_MAX], buf, BLOCKSIZE))
            return false;
    }
    else
    {}
    return true;
}

bool FSController::ReadFileToBuf(const iNode& cur, int start, int len, char* buf)
{
    if (start > cur.size - 1 || start + len - 1 > cur.size - 1)
        return false;
    return true;
}
