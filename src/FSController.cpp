#include "FSController.h"
#include "VHDController.h"
#include "BSFSParams.h"
#include "MBR.h"
#include "iNode.h"
#include "SFD.h"
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

bool FSController::GetContentInDir(const iNode& curDir, SFD* rst)
{
    if(!ReadFileToBuf(curDir, 0, curDir.size, (char*)rst))
        return false;
    return true;
}

bool FSController::FindContentInDir(const SFD* DirSet, const int len, char* name, int* rst)
{
    for(int i = 0; i < len; i++)
    {
        if(strcmp(DirSet[i].name, name) == 0)
        {
            *rst = i;
            return true;
        }
    }
    return false;
}

// curDir: current directory
// path: path to parse
// last: whether parse to the last level of path or not
bool FSController::ParsePath(const iNode& curDir, char* path, bool last, iNode* rst)
{
    iNode nowiNode, tmpiNode;
    int pp = 0, totLinkCnt = 0;
    if(path[0] == '/')
    {
        if(!vhdc.ReadBlock(ROOTDIRiNODE, (char*)&nowiNode, sizeof(iNode)))
            return false;
        pp++;
    }
    else memcpy((char*)&nowiNode, (char*)&curDir, sizeof(iNode));

    char buf[FILENAME_MAXLEN + 10];
    int bufp, target;
    while(true)
    {
        bufp = 0;
        while(path[pp] == '/') pp++;
        while(path[pp] != '/')
        {
            if(path[pp] == '\0') break;
            buf[bufp]=path[pp];
            bufp++;
            pp++;
        }

        // Skip last level if last == false
        if((!last) && (path[pp] == '\0')) break;

        buf[bufp] = '\0';
        if(bufp == 0) break;

        int subDirnum = nowiNode.size / sizeof(SFD);
        SFD* DirSet = new SFD[subDirnum];
        if(!GetContentInDir(nowiNode, DirSet))
        {
            delete[] DirSet;
            return false;
        }
        if(FindContentInDir(DirSet, subDirnum, buf, &target))
        {
            if(!vhdc.ReadBlock(DirSet[target].inode, (char*)&tmpiNode, sizeof(iNode)))
            {
                delete[] DirSet;
                return false;
            }
            // If the last level of the path is a file -> true
            // else -> false
            if(!(tmpiNode.mode & DIRFLAG) && (pp != '\0'))
                return false;
            // Follow symbolic link
            if(tmpiNode.mode & SYNLINKFLAG)
            {
                totLinkCnt++;
                if(totLinkCnt > MAXFOLLOWLINK) return false;

                char* synlink = new char[tmpiNode.size];
                if(!ReadFileToBuf(tmpiNode, 0, tmpiNode.size, synlink))
                {
                    delete[] synlink;
                    return false;
                }
                if(synlink[0] == '/')
                {
                    if(!vhdc.ReadBlock(ROOTDIRiNODE, (char*)&nowiNode, sizeof(iNode)))
                    {
                        delete[] synlink;
                        return false;
                    }
                }
                int len = strlen(path);
                char* newpath = new char[len + tmpiNode.size + 10];
                int newlen = 0;
                for(int i = 0; i < len; i++)
                {
                    newpath[newlen++] = path[i];
                    if(i == pp)
                    {
                        for(int j = 0;  j < (int)tmpiNode.size; j++)
                            newpath[newlen++] = synlink[j];
                        newpath[newlen++] = '/';
                    }
                }
                strcpy(path, newpath);
                delete[] synlink;
                delete[] newpath;
                continue;
            }
            memcpy((char*)&nowiNode, (char*)&tmpiNode, sizeof(iNode));
        }
        else return false;
    }

    memcpy((char*)rst, (char*)&nowiNode, sizeof(iNode));
    return true;
}
