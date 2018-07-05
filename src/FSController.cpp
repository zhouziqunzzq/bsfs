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
    return this->vhdc.ReadBlock(MBRBLOCK, (char*)&mbr) && mbr.formatFlag;
}

bool FSController::GetBIDByFOff(const iNode& cur, int foff, int* rst)
{
    int startBlock = foff / BLOCKSIZE;
    if (startBlock <= INODE_DIRECT_MAX)
        *rst = cur.data[startBlock];
    else if (startBlock <= (int)(INODE_DIRECT_MAX + BLOCKSIZE / sizeof(int)))
    {
        char indir1[BLOCKSIZE];
        if(!vhdc.ReadBlock(cur.data[INODE_INDIR1_MAX], indir1, BLOCKSIZE))
            return false;
        char* toffset = indir1 + (startBlock - DIRECT_BLOCK_CNT) * sizeof(int);
        memcpy(rst, toffset, sizeof(int));
    }
    else
    {
        // Find level one indirect index block
        char indir1[BLOCKSIZE];
        if(!vhdc.ReadBlock(cur.data[INODE_INDIR2_MAX], indir1, BLOCKSIZE))
            return false;
        int offInIndir2 = (startBlock - DIRECT_BLOCK_CNT - INDIR1_BLOCK_CNT) /
            (BLOCKSIZE / sizeof(int));
        char* toffset = indir1 + offInIndir2 * sizeof(int);
        int indir2ID;
        memcpy(&indir2ID, toffset, sizeof(int));
        // Find level two indirect index block
        char indir2[BLOCKSIZE];
        if(!vhdc.ReadBlock(indir2ID, indir2, BLOCKSIZE))
            return false;
        toffset = indir2 + ((startBlock - DIRECT_BLOCK_CNT - INDIR1_BLOCK_CNT) %
                            (BLOCKSIZE / sizeof(int))) * sizeof(int);
        memcpy(rst, toffset, sizeof(int));
    }
    return true;
}

bool FSController::ReadFileToBuf(const iNode& cur, int start, int len, char* buf)
{
    if (start > (int)cur.size - 1 || start + len - 1 > (int)cur.size - 1)
        return false;
    char* tbuf = new char[len + 2 * BLOCKSIZE];
    int filep = start;  // File reader pointer
    char* bufp = tbuf;   // Buffer writer pointer
    int tBID;
    while (filep < start + len + BLOCKSIZE - (start + len) % BLOCKSIZE)
    {
        if (!GetBIDByFOff(cur, filep, &tBID)) return false;
        if (!this->vhdc.ReadBlock(tBID, bufp, BLOCKSIZE)) return false;
        filep += BLOCKSIZE;
        bufp += BLOCKSIZE;
    }
    char* s = tbuf + (start % BLOCKSIZE);
    //char* e = tbuf + start % BLOCKSIZE + len;
    memcpy(buf, s, len);
    delete[] tbuf;
    return true;
}
