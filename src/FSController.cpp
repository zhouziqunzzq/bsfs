#include "FSController.h"
#include "VHDController.h"
#include "BSFSParams.h"
#include "MBR.h"
#include "iNode.h"
#include "SFD.h"
#include <cstring>
#include <iostream>
#include <cmath>
#include <ctime>

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
    fbc.SetFullFlag();
    for(int i = DATABLOCK_MIN; i <= DATABLOCK_MAX; i++)
        if(!fbc.Recycle(i)) return false;
    if(!fbc.SaveSuperBlock()) return false;

    // Fomat iNode Super Block and construct GPL
    ifbc.SetFullFlag();
    for(int i = iNODEBLOCK_MIN + 1; i <= iNODEBLOCK_MAX; i++)   // skip iNode of /
        if(!ifbc.Recycle(i)) return false;
    if(!ifbc.SaveSuperBlock()) return false;

    return true;
}

bool FSController::IsFormat()
{
    MBR mbr;
    return this->vhdc.ReadBlock(MBRBLOCK, (char*)&mbr) && mbr.formatFlag;
}

bool FSController::GetBIDByFOff(const iNode& cur, int foff, bid_t* rst)
{
    int startBlock = foff / BLOCKSIZE;
    if (startBlock <= INODE_DIRECT_MAX)
        *rst = cur.data[startBlock];
    else if (startBlock <= (int)(INODE_DIRECT_MAX + BLOCKSIZE / sizeof(bid_t)))
    {
        char indir1[BLOCKSIZE];
        if(!vhdc.ReadBlock(cur.data[INODE_INDIR1_MAX], indir1, BLOCKSIZE))
            return false;
        char* toffset = indir1 + (startBlock - DIRECT_BLOCK_CNT) * sizeof(bid_t);
        memcpy(rst, toffset, sizeof(bid_t));
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
        memcpy(rst, toffset, sizeof(bid_t));
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
    bid_t tBID;
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

// Append new blocks count by blockCnt to a file identified by iNode& cur
// and update its iNode
bool FSController::AppendBlocksToFile(iNode& cur, int blockCnt)
{
    // Check if blockCnt is bigger than the max block cnt
    if (cur.blocks + blockCnt > MAX_BLOCK_CNT) return false;

    bid_t dataBid = 0, directBid = 0, indir1Bid = 0, indir2Bid = 0;
    char indir1Buf[BLOCKSIZE], indir2Buf[BLOCKSIZE];
    bid_t indir1BufLoadBid = 0, indir2BufLoadBid = 0;
    memset(indir1Buf, 0, sizeof(BLOCKSIZE));
    memset(indir2Buf, 0, sizeof(BLOCKSIZE));
    // Append new blocks one by one
    for (int i = (int)cur.blocks; i < (int)cur.blocks + blockCnt; i++)
    {
        if (i < DIRECT_BLOCK_CNT)   // direct index
        {
            if (!this->fbc.Distribute(&directBid)) return false;
            cur.data[i] = directBid;
        }
        else if (i == DIRECT_BLOCK_CNT) // level 1 indirect index(new)
        {
            // Distribute level 1 indirect index block
            if (!this->fbc.Distribute(&indir1Bid)) return false;
            cur.data[INODE_INDIR1_MAX] = indir1Bid;
            indir1BufLoadBid = indir1Bid;
            // and distribute 1 data block
            if (!this->fbc.Distribute(&dataBid)) return false;
            memcpy(indir1Buf, (char*)&dataBid, sizeof(bid_t));
            if (!vhdc.WriteBlock(indir1Bid, indir1Buf, BLOCKSIZE)) return false;
        }
        else if (i < (int)(DIRECT_BLOCK_CNT + INDIR1_BLOCK_CNT))   // level 1 indirect index
        {
            // Load level 1 indirect index block if needed
            if (indir1BufLoadBid != cur.data[INODE_INDIR1_MAX])
            {
                indir1Bid = cur.data[INODE_INDIR1_MAX];
                if (!this->vhdc.ReadBlock(indir1Bid, indir1Buf)) return false;
                indir1BufLoadBid = cur.data[INODE_INDIR1_MAX];
            }
            // and distribute 1 data block
            if (!this->fbc.Distribute(&dataBid)) return false;
            memcpy(indir1Buf + (i - DIRECT_BLOCK_CNT) * sizeof(bid_t),
                   (char*)&dataBid, sizeof(bid_t));
            if (!vhdc.WriteBlock(indir1Bid, indir1Buf, BLOCKSIZE)) return false;
        }
        else if (i == DIRECT_BLOCK_CNT + INDIR1_BLOCK_CNT)  // level 2 indirect index(new)
        {
            // Distribute level 2 indirect index block
            if (!this->fbc.Distribute(&indir2Bid)) return false;
            cur.data[INODE_INDIR2_MAX] = indir2Bid;
            indir2BufLoadBid = indir2Bid;
            // Distribute level 1 indirect index block
            memset(indir1Buf, 0, BLOCKSIZE);
            if (!this->fbc.Distribute(&indir1Bid)) return false;
            memcpy(indir2Buf, (char*)&indir1Bid, sizeof(bid_t));
            if (!vhdc.WriteBlock(indir2Bid, indir2Buf, BLOCKSIZE)) return false;
            // and distribute 1 data block
            if (!this->fbc.Distribute(&dataBid)) return false;
            memcpy(indir1Buf, (char*)&dataBid, sizeof(bid_t));
            if (!vhdc.WriteBlock(indir1Bid, indir1Buf, BLOCKSIZE)) return false;
            indir1BufLoadBid = indir1Bid;
        }
        else    // level 2 indirect index
        {
            // Load level 2 indirect index block if needed
            if (indir2BufLoadBid != cur.data[INODE_INDIR2_MAX])
            {
                indir2Bid = cur.data[INODE_INDIR2_MAX];
                if (!this->vhdc.ReadBlock(indir2Bid, indir2Buf)) return false;
                indir2BufLoadBid = cur.data[INODE_INDIR2_MAX];
            }
            // Load level 1 indirect index block or
            // append new level 1 indirect index block if needed
            unsigned int indir21Off = i - DIRECT_BLOCK_CNT - INDIR1_BLOCK_CNT;
            if (indir21Off % (BLOCKSIZE / sizeof(bid_t)) == 0)
            {
                // Distribute level 1 indirect index block
                memset(indir1Buf, 0, BLOCKSIZE);
                if (!this->fbc.Distribute(&indir1Bid)) return false;
                indir1BufLoadBid = indir1Bid;
                memcpy(indir2Buf + (indir21Off / (BLOCKSIZE / sizeof(bid_t))) * sizeof(bid_t),
                       (char*)&indir1Bid, sizeof(bid_t));
                if (!this->vhdc.WriteBlock(indir2Bid, indir2Buf, BLOCKSIZE)) return false;
            }
            else
            {
                // Load level 1 indirect index block if needed
                bid_t indir21Bid = 0;
                // toff is the address of the level 1 indirect index block ID of
                // the level 2 indirect index
                char* toff = indir2Buf + (indir21Off /
                       (BLOCKSIZE / sizeof(bid_t))) * sizeof(bid_t);
                memcpy((char*)&indir21Bid, toff, sizeof(bid_t));
                if (indir1BufLoadBid != indir21Bid)
                {
                    if (!this->vhdc.ReadBlock(indir21Bid, indir1Buf)) return false;
                    indir1BufLoadBid = indir21Bid;
                }
            }
            // and distribute 1 data block
            if (!this->fbc.Distribute(&dataBid)) return false;
            memcpy(indir1Buf + (indir21Off % (BLOCKSIZE / sizeof(bid_t))) * sizeof(bid_t),
                   (char*)&dataBid, sizeof(bid_t));
            if (!this->vhdc.WriteBlock(indir1BufLoadBid, indir1Buf, BLOCKSIZE)) return false;
        }
    }
    // Update iNode
    cur.blocks += blockCnt;
    if (!this->vhdc.WriteBlock(cur.bid, (char*)&cur, sizeof(iNode))) return false;
    return true;
}

bool FSController::WriteFileFromBuf(iNode& cur, int start, int len, char* buf)
{
    // Append new blocks if needed
    int deltaBlocks = ceil(((float)start + len) / (float)(BLOCKSIZE)) - cur.blocks;
    if (deltaBlocks > 0)
    {
        if (!this->AppendBlocksToFile(cur, deltaBlocks)) return false;
    }
    // Write buf to file block by block
    int startBlock = start / BLOCKSIZE;
    int endBlock = (start + len) / BLOCKSIZE;
    int nowBlock = startBlock;
    char* bufp = buf;
    bid_t curBID = 0;
    int filep = start;
    char rbuf[BLOCKSIZE];

    if (startBlock == endBlock) // Write in one block
    {
        if (!GetBIDByFOff(cur, start, &curBID)) return false;
        if (!this->vhdc.ReadBlock(curBID, rbuf, BLOCKSIZE)) return false;
        memcpy(rbuf + start % BLOCKSIZE, buf, len);
        if (!this->vhdc.WriteBlock(curBID, rbuf, BLOCKSIZE)) return false;
    }
    else    // Write in multiple blocks
    {
        while (filep < start + len + BLOCKSIZE - (start + len) % BLOCKSIZE)
        {
            if (!GetBIDByFOff(cur, filep, &curBID)) return false;
            if (!this->vhdc.ReadBlock(curBID, rbuf, BLOCKSIZE)) return false;
            if (nowBlock == startBlock) // First block
            {
                int toff = start % BLOCKSIZE;
                int tlen = BLOCKSIZE - toff;
                memcpy(rbuf + toff, bufp, tlen);
                bufp += tlen;
            }
            else if (nowBlock == endBlock)  // Last block
            {
                int tlen = (start + len) % BLOCKSIZE;
                memcpy(rbuf, bufp, tlen);
                bufp += tlen;
            }
            else    // Middle block
            {
                memcpy(rbuf, bufp, BLOCKSIZE);
                bufp += BLOCKSIZE;
            }
            if (!this->vhdc.WriteBlock(curBID, rbuf, BLOCKSIZE)) return false;
            filep += BLOCKSIZE;
            nowBlock++;
        }
    }
    // Update iNode
    cur.mtime = time(nullptr);
    cur.atime = time(nullptr);
    if (start + len > (int)cur.size)
    {
        cur.size = start + len;
        cur.bytes = (start + len) % BLOCKSIZE;
        if (!this->vhdc.WriteBlock(cur.bid, (char*)&cur, sizeof(iNode))) return false;
    }
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
                path[newlen++] = '\0';
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

bool FSController::InitDirSFDList(iNode& cur, bid_t parentBid)
{
    SFD sfdList[2];
    // Dot denode itself
    strcpy(sfdList[0].name, ".");
    sfdList[0].inode = cur.bid;
    // Dot Dot denode its parent
    strcpy(sfdList[1].name, "..");
    sfdList[1].inode = parentBid;

    return this->WriteFileFromBuf(cur, 0, sizeof(SFD) * 2, (char*)&sfdList);
}

bool FSController::CreateRootDir()
{
    // Create RootDir iNode
    iNode rootiNode;
    rootiNode.bid = ROOTDIRiNODE;
    rootiNode.parent = ROOTDIRiNODE;
    strcpy(rootiNode.name, "/");
    rootiNode.mode = DIRFLAG | OWNER_RFLAG | OWNER_WFLAG | OWNER_XFLAG | PUBLIC_RFLAG |
        PUBLIC_XFLAG;
    rootiNode.nlink = 1;
    rootiNode.uid = ROOT_UID;
    rootiNode.size = 0;
    rootiNode.atime = rootiNode.mtime = time(nullptr);
    rootiNode.blocks = 0;
    rootiNode.bytes = 0;
    if (!this->vhdc.WriteBlock(ROOT_INODE, (char*)&rootiNode, sizeof(iNode)))
        return false;
    // Create RootDir SFD List
    if (!this->InitDirSFDList(rootiNode, ROOTDIRiNODE))
        return false;
    return true;
}

bool FSController::TouchFile(iNode& curDir, char* name, char mode, int ownerUid, iNode* rst)
{
    // Read current dir
    SFD* dir = new SFD[curDir.size / sizeof(SFD)];
    if (!this->GetContentInDir(curDir, dir))
    {
        delete[] dir;
        return false;
    }
    int findRst;
    // Check if name already exists
    if (this->FindContentInDir(dir, curDir.size / sizeof(SFD), name, &findRst))
    {
        delete[] dir;
        return false;
    }
    // Create new iNode
    iNode newiNode;
    if (!this->ifbc.Distribute(&newiNode.bid))
    {
        delete[] dir;
        return false;
    }
    newiNode.parent = curDir.bid;
    strcpy(newiNode.name, name);
    newiNode.mode = mode;
    newiNode.nlink = 1;
    newiNode.uid = ownerUid;
    newiNode.size = 0;
    newiNode.atime = newiNode.mtime = time(nullptr);
    newiNode.blocks = 0;
    newiNode.bytes = 0;
    // Save new iNode
    if (!this->vhdc.WriteBlock(newiNode.bid, (char*)&newiNode, sizeof(iNode)))
    {
        delete[] dir;
        return false;
    }
    memcpy((char*)rst, (char*)&newiNode, sizeof(iNode));
    // Append new SFD
    SFD newSFD;
    strcpy(newSFD.name, name);
    newSFD.inode = newiNode.bid;
    if (!this->WriteFileFromBuf(curDir, curDir.size, sizeof(SFD), (char*)&newSFD))
    {
        delete[] dir;
        return false;
    }
    // Clean up
    delete[] dir;
    return true;
}

bool FSController::CreateSubDir(iNode& curDir, char* name, char mode, int ownerUid)
{
    iNode newiNode;
    if (!this->TouchFile(curDir, name, mode | DIRFLAG, ownerUid, &newiNode))
        return false;
    return this->InitDirSFDList(newiNode, curDir.bid);
}

bool FSController::GetiNodeByID(bid_t id, iNode* rst)
{
    return this->vhdc.ReadBlock(id, (char*)rst, sizeof(iNode));
}

void FSController::GetAbsDir(const iNode& cur, char* dir)
{
    // TODO
    return;
}
