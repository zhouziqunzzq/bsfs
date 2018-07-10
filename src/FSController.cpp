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
    // Format iNode Super Block and construct GPL
    ifbc.SetFullFlag();
    for(int i = iNODEBLOCK_MIN + 1; i <= iNODEBLOCK_MAX; i++)   // skip iNode of /
        if(!ifbc.Recycle(i)) return false;
    if(!ifbc.SaveSuperBlock()) return false;
    // Create /
    if (!this->CreateRootDir()) return false;
    // Create /home
    iNode rootiNode;
    char homeName[] = "home";
    if (!this->GetiNodeByID(ROOTDIRiNODE, &rootiNode)) return false;
    iNode homeiNode;
    if (!this->CreateSubDir(rootiNode, homeName,
        OWNER_ALLFLAG | PUBLIC_RFLAG | PUBLIC_XFLAG, ROOT_UID, &homeiNode))
        return false;
    // Create /etc
    char etcName[] = "etc";
    iNode etciNode;
    if (!this->CreateSubDir(rootiNode, etcName,
        OWNER_ALLFLAG | PUBLIC_RFLAG | PUBLIC_XFLAG, ROOT_UID, &etciNode))
        return false;
    // Create USER_FILE
    char ufName[] = USER_FILENAME;
    iNode ufiNode;
    if (!this->Touch(etciNode, ufName, OWNER_ALLFLAG, ROOT_UID, &ufiNode))
        return false;
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
    memset(indir1Buf, 0, BLOCKSIZE);
    memset(indir2Buf, 0, BLOCKSIZE);
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
    if (!this->SaveiNodeByID(cur.bid, cur)) return false;
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
    }
    if (!this->SaveiNodeByID(cur.bid, cur)) return false;
    return true;
}

bool FSController::GetContentInDir(const iNode& curDir, SFD* rst)
{
    if (!(curDir.mode & DIRFLAG)) return false;
    if(!ReadFileToBuf(curDir, 0, curDir.size, (char*)rst))
        return false;
    return true;
}

bool FSController::FindContentInDir(const SFD* DirSet, const int len, const char* name, int* rst)
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
// followLastSyn: whether follow the last level of synlink or not
bool FSController::ParsePath(const iNode& curDir, char* path,
                             bool last, iNode* rst, bool followLastSyn)
{
    iNode nowiNode, tmpiNode;
    int pp = 0, totLinkCnt = 0;
    if(path[0] == '/')
    {
        if (!this->GetiNodeByID(ROOTDIRiNODE, &nowiNode))
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
            if (!this->GetiNodeByID(DirSet[target].inode, &tmpiNode))
            {
                delete[] DirSet;
                return false;
            }
            // Follow symbolic link unless reach last level
            if((tmpiNode.mode & SYNLINKFLAG) && (
                (path[pp] != '\0') || followLastSyn))
            {
                totLinkCnt++;
                if(totLinkCnt > MAXFOLLOWLINK) return false;

                char* synlink = new char[tmpiNode.size + 1];
                synlink[tmpiNode.size] = '\0';
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
                for(int i = 0; i <= len; i++)
                {
                    if (path[i] != '\0')
                        newpath[newlen++] = path[i];
                    if(i == pp)
                    {
                        for(int j = 0;  j < (int)tmpiNode.size; j++)
                            newpath[newlen++] = synlink[j];
                        newpath[newlen++] = '/';
                    }
                }
                newpath[newlen] = '\0';
                strcpy(path, newpath);
                path[newlen] = '\0';

                delete[] synlink;
                delete[] newpath;
                continue;
            }
            // If the last level of the path is a file -> true
            // else -> false
            if(!(tmpiNode.mode & DIRFLAG) && (path[pp] != '\0'))
                return false;
            memcpy((char*)&nowiNode, (char*)&tmpiNode, sizeof(iNode));
        }
        else
        {
            delete[] DirSet;
            return false;
        }
        delete[] DirSet;
    }

    memcpy((char*)rst, (char*)&nowiNode, sizeof(iNode));
    return true;
}

bool FSController::InitDirSFDList(iNode& cur, bid_t parentBid)
{
    SFD sfdList[2];
    // Dot denode itself
    strcpy(sfdList[0].name, DOT);
    sfdList[0].inode = cur.bid;
    // Dot Dot denode its parent
    strcpy(sfdList[1].name, DOTDOT);
    sfdList[1].inode = parentBid;

    return this->WriteFileFromBuf(cur, 0, sizeof(SFD) * 2, (char*)sfdList);
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
    // Create RootDir SFD List
    if (!this->InitDirSFDList(rootiNode, ROOTDIRiNODE))
        return false;
    // Save RootDir iNode
    if (!this->SaveiNodeByID(ROOTDIRiNODE, rootiNode))
        return false;
    return true;
}

bool FSController::Touch(iNode& curDir, char* name, char mode, int ownerUid, iNode* rst)
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
    iNode newiNode;    //parent.size += sizeof(SFD);
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
    if (!this->SaveiNodeByID(newiNode.bid, newiNode))
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
    // Update curDir iNode
    curDir.mtime = time(nullptr);
    if (!this->SaveiNodeByID(curDir.bid, curDir)) return false;
    // Clean up
    delete[] dir;
    return true;
}

bool FSController::CreateSubDir(iNode& curDir, char* name, char mode, uid_t ownerUid, iNode* rst)
{
    if (!this->Touch(curDir, name, mode | DIRFLAG, ownerUid, rst))
        return false;
    if (!this->InitDirSFDList(*rst, curDir.bid))
        return false;
    return this->SaveiNodeByID(curDir.bid, curDir);
}

bool FSController::GetiNodeByID(bid_t id, iNode* rst)
{
    return this->vhdc.ReadBlock(id, (char*)rst, sizeof(iNode));
}

bool FSController::SaveiNodeByID(bid_t id, const iNode& inode)
{
    return this->vhdc.WriteBlock(id, (char*)&inode, sizeof(iNode));
}

void FSController::GetAbsDir(const iNode& cur, char* dir)
{
    char rootName[] = "/";
    if (strcmp(cur.name, rootName) == 0)
    {
        strcpy(dir, rootName);
        return;
    }
    string ts;
    iNode now;
    memcpy((char*)&now, (char*)&cur, sizeof(iNode));
    while (strcmp(now.name, "/") != 0)
    {
        string tt(now.name);
        ts = "/" + tt + ts;
        this->GetiNodeByID(now.parent, &now);
    }
    strcpy(dir, ts.c_str());
    return;
}

bool FSController::DeleteDirectBlocks(const iNode& cur)
{
    int maxi = cur.blocks > DIRECT_BLOCK_CNT ? DIRECT_BLOCK_CNT : cur.blocks;
    for (bid_t i = 0; i < (unsigned int)maxi; i++)
    {
        if (!this->fbc.Recycle(cur.data[i])) return false;
    }
    return true;
}

bool FSController::DeleteIndir1Blocks(const iNode& cur)
{
    // Readin indir1 block
    bid_t indir1Buf[BLOCKSIZE / sizeof(bid_t)];
    if (!this->vhdc.ReadBlock(cur.data[INODE_INDIR1_MAX], (char*)indir1Buf))
        return false;
    // Recycle data blocks
    int maxi = cur.blocks - DIRECT_BLOCK_CNT > INDIR1_BLOCK_CNT ?
        (int)INDIR1_BLOCK_CNT : (int)cur.blocks - DIRECT_BLOCK_CNT;
    for (int i = 0; i < maxi; i++)
    {
        if (!this->fbc.Recycle(indir1Buf[i])) return false;
    }
    // Recycle indir1 block
    return this->fbc.Recycle(cur.data[INODE_INDIR1_MAX]);
}

bool FSController::DeleteIndir2Blocks(const iNode& cur)
{
    // Readin indir2 block
    bid_t indir2Buf[BLOCKSIZE / sizeof(bid_t)];
    if (!this->vhdc.ReadBlock(cur.data[INODE_INDIR2_MAX], (char*)indir2Buf))
        return false;
    // Calc total indir1 count and last indir1 count
    int indir1Cnt = (cur.blocks - DIRECT_BLOCK_CNT - INDIR1_BLOCK_CNT)
        / (BLOCKSIZE / sizeof(bid_t));
    int lastIndir1Cnt = (cur.blocks - DIRECT_BLOCK_CNT - INDIR1_BLOCK_CNT)
        % (BLOCKSIZE / sizeof(bid_t));
    // Do recycle
    bid_t indir1Buf[BLOCKSIZE / sizeof(bid_t)];
    for (int i = 0; i < indir1Cnt; i++)
    {
        // Readin indir1 block
        if (!this->vhdc.ReadBlock(indir2Buf[i], (char*)&indir1Buf))
            return false;
        // Recycle data block (end with 0)
        int maxj = (i == indir1Cnt - 1 ? lastIndir1Cnt : BLOCKSIZE / sizeof(bid_t));
        for (int j = 0; j < maxj; j++)
        {
            if (!this->fbc.Recycle(indir1Buf[j])) return false;
        }
        // Recycle indir1 block
        if (!this->fbc.Recycle(indir2Buf[i])) return false;
    }
    // Recycle indir2 block
    return this->fbc.Recycle(cur.data[INODE_INDIR2_MAX]);
}

bool FSController::DeleteSFDEntry(iNode& cur)
{
    // Get parent iNode
    iNode piNode;
    if (!this->GetiNodeByID(cur.parent, &piNode)) return false;
    if (!(piNode.mode & DIRFLAG)) return false;
    // Get SFD List
    SFD* SFDList = new SFD[piNode.size / sizeof(SFD)];
    if (!this->ReadFileToBuf(piNode, 0, piNode.size, (char*)SFDList))
    {
        delete[] SFDList;
        return false;
    }
    // Delete target SFD
    int newp = 0;
    SFD* newSFDList = new SFD[piNode.size / sizeof(SFD) - 1];
    bool finded = false;
    for (int i = 0; i < (int)(piNode.size / sizeof(SFD)); i++)
    {
        if (strcmp(SFDList[i].name, cur.name) == 0)
        {
            finded = true;
            continue;
        }
        else
        {
            memcpy((char*)&newSFDList[newp], (char*)&SFDList[i], sizeof(SFD));
            newp++;
        }
    }
    if (!finded)
    {
        delete[] SFDList;
        delete[] newSFDList;
        return false;
    }
    // Save new SFD List
    if (!this->WriteFileFromBuf(piNode, 0, piNode.size - sizeof(SFD), (char*)newSFDList))
    {
        delete[] SFDList;
        delete[] newSFDList;
        return false;
    }
    // Update parent iNode
    piNode.size -= sizeof(SFD);
    if (!this->SaveiNodeByID(piNode.bid, piNode))
    {
        delete[] SFDList;
        delete[] newSFDList;
        return false;
    }
    delete[] newSFDList;
    delete[] SFDList;
    return true;
}

bool FSController::AppendSFDEntry(iNode& parent, const SFD& newSFD)
{
    // Only for dir
    if (!(parent.mode & DIRFLAG)) return false;
    // Get SFD List
    SFD* SFDList = new SFD[parent.size / sizeof(SFD)];
    if (!this->ReadFileToBuf(parent, 0, parent.size, (char*)SFDList))
    {
        delete[] SFDList;
        return false;
    }
    // Check if already exists
    int rst = 0;
    if (this->FindContentInDir(SFDList, parent.size / sizeof(SFD), newSFD.name, &rst))
    {
        delete[] SFDList;
        return false;
    }
    // Append new SFD entry
    if (!this->WriteFileFromBuf(parent, parent.size, sizeof(SFD), (char*)&newSFD))
    {
        delete[] SFDList;
        return false;
    }
    delete[] SFDList;
    return true;
}

bool FSController::DeleteFile(iNode& cur)
{
    // Not for directory
    if (cur.mode & DIRFLAG) return false;
    // Recycle data blocks and iNode if nlink == 1
    // (if nlink > 1, then there're other hard links,
    //  which means we can't recycle its data blocks or iNode yet)
    if (cur.nlink == 1)
    {
        if (cur.blocks <= DIRECT_BLOCK_CNT)
        {
            if(!this->DeleteDirectBlocks(cur))
                return false;
        }
        else if (cur.blocks <= DIRECT_BLOCK_CNT + INDIR1_BLOCK_CNT)
        {
            if (!this->DeleteDirectBlocks(cur) & this->DeleteIndir1Blocks(cur))
                return false;
        }
        else
        {
            if (!this->DeleteDirectBlocks(cur) & this->DeleteIndir1Blocks(cur) &
                this->DeleteIndir2Blocks(cur))
                return false;
        }
    }
    // Delete SFD in parent
    if (!this->DeleteSFDEntry(cur)) return false;
    // Recycle iNode block
    if (!this->ifbc.Recycle(cur.bid)) return false;
    return true;
}

bool FSController::DeleteDir(iNode& cur)
{
    // Only for dir
    if (!(cur.mode & DIRFLAG)) return false;
    // Readin SFD List
    iNode nowiNode;
    SFD* SFDList = new SFD[cur.size / sizeof(SFD)];
    if (!GetContentInDir(cur, SFDList))
    {
        delete[] SFDList;
        return false;
    }
    // Iterate over SFD List
    // Note: it's forbidden to create a hard link of a dir,
    // so we don't need to consider that.
    for (int i = 0; i < (int)(cur.size / sizeof(SFD)); i++)
    {
        // Ignore . or ..
        if (strcmp(SFDList[i].name, DOT) == 0 ||
            strcmp(SFDList[i].name, DOTDOT) == 0)
            continue;
        // Readin iNode
        if (!this->GetiNodeByID(SFDList[i].inode, &nowiNode))
        {
            delete[] SFDList;
            return false;
        }
        if (nowiNode.mode & DIRFLAG)    // Delete subdir recursively
        {
            if (!this->DeleteDir(nowiNode))
            {
                delete[] SFDList;
                return false;
            }
        }
        else    // Delete regular file
        {
            if (!this->DeleteFile(nowiNode))
            {
                delete[] SFDList;
                return false;
            }
        }
    }
    // Delete SFD in parent
    if (!this->DeleteSFDEntry(cur))
    {
        delete[] SFDList;
        return false;
    }
    // Recycle iNode block
    if (!this->ifbc.Recycle(cur.bid))
    {
        delete[] SFDList;
        return false;
    }
    delete[] SFDList;
    return true;
}

bool FSController::ChangeMode(iNode& cur, char mode)
{
    cur.mode = mode;
    return this->SaveiNodeByID(cur.bid, cur);
}

// Copy file identified by src into a dir identified by des with given name
bool FSController::CopyFile(const iNode& src, iNode& des, char* name, uid_t uid)
{
    // Only for file
    if (src.mode & DIRFLAG) return false;
    // Touch new file with given name
    iNode newiNode;
    if (!this->Touch(des, name, src.mode, uid, &newiNode))
        return false;
    // If the size of src file is 0, just touch it
    if (src.size == 0) return true;
    // Readin src file
    char* buf = new char[src.size];
    if (!this->ReadFileToBuf(src, 0, src.size, buf))
    {
        delete[] buf;
        return false;
    }
    // Write to des
    if (!this->WriteFileFromBuf(newiNode, 0, src.size, buf))
    {
        delete[] buf;
        return false;
    }
    delete[] buf;
    return true;
}

bool FSController::CopyDir(const iNode& src, iNode& des, char* name, uid_t uid)
{
    // Only for dir
    if (!(src.mode & DIRFLAG)) return false;
    // Create subdir
    iNode newdiriNode;
    if (!this->CreateSubDir(des, name, src.mode, uid, &newdiriNode))
        return false;
    // Readin SFD List
    iNode nowiNode;
    SFD* SFDList = new SFD[src.size / sizeof(SFD)];
    if (!GetContentInDir(src, SFDList))
    {
        delete[] SFDList;
        return false;
    }
    // Iterate over srcdir
    for (int i = 0; i < (int)(src.size / sizeof(SFD)); i++)
    {
        // Ignore . or ..
        if (strcmp(SFDList[i].name, DOT) == 0 || strcmp(SFDList[i].name, DOTDOT) == 0)
            continue;
        // Readin iNode
        if (!this->GetiNodeByID(SFDList[i].inode, &nowiNode))
        {
            delete[] SFDList;
            return false;
        }
        if (nowiNode.mode & DIRFLAG)    // Copy subdir recursively
        {
            if (!this->CopyDir(nowiNode, newdiriNode, nowiNode.name, uid))
            {
                delete[] SFDList;
                return false;
            }
        }
        else    // Copy regular file
        {
            if (!this->CopyFile(nowiNode, newdiriNode, nowiNode.name, uid))
            {
                delete[] SFDList;
                return false;
            }
        }
    }
    delete[] SFDList;
    return true;
}

bool FSController::Copy(const iNode& src, iNode& des, char* name, uid_t uid)
{
    if (src.mode & DIRFLAG)
        return this->CopyDir(src, des, name, uid);
    else
        return this->CopyFile(src, des, name, uid);
}

bool FSController::Move(iNode& src, iNode& des, char* name)
{
    // Delete SFD entry from old parent dir
    if (!this->DeleteSFDEntry(src)) return false;
    // Reload iNode if needed
    if (src.parent == des.bid)
    {
        if (!this->GetiNodeByID(des.bid, &des))
            return false;
    }
    // Append new SFD entry
    SFD newSFD;
    strcpy(newSFD.name, name);
    newSFD.inode = src.bid;
    if (!this->AppendSFDEntry(des, newSFD)) return false;
    // Update src iNode
    strcpy(src.name, name);
    src.parent = des.bid;
    if (!this->SaveiNodeByID(src.bid, src)) return false;
    // Update src SFD List if src is a directory
    if (src.mode & DIRFLAG)
    {
        SFD* SFDList = new SFD[src.size / sizeof(SFD)];
        if (!this->GetContentInDir(src, SFDList))
        {
            delete[] SFDList;
            return false;
        }
        int rst;
        if (!this->FindContentInDir(SFDList, src.size / sizeof(SFD), DOTDOT, &rst))
        {
            delete[] SFDList;
            return false;
        }
        SFDList[rst].inode = des.bid;
        if (!this->WriteFileFromBuf(src, 0, src.size, (char*)SFDList))
        {
            delete[] SFDList;
            return false;
        }
        delete[] SFDList;
    }
    return true;
}

bool FSController::LinkH(iNode& src, iNode& des, char* name)    // Fake
{
    // Only for regular file
    if (src.mode & DIRFLAG) return false;
    // Update src iNode
    src.nlink++;
    if (!this->SaveiNodeByID(src.bid, src)) return false;
    // Touch new file
    iNode newiNode;
    if (!this->Touch(des, name, src.mode, src.uid, &newiNode))
        return false;
    // Copy iNode info
    newiNode.size = src.size;
    newiNode.mtime = src.mtime;
    newiNode.atime = src.atime;
    newiNode.blocks = src.blocks;
    newiNode.bytes = src.bytes;
    memcpy(newiNode.data, src.data, INODE_DATASIZE);
    // Save iNode
    if (!this->SaveiNodeByID(newiNode.bid, newiNode)) return false;
    return true;
}

bool FSController::LinkS(char* src, iNode& des, char* name, uid_t uid)
{
    // Touch new file
    iNode newiNode;
    if (!this->Touch(des, name, FILE_DEFAULT_FLAG | SYNLINKFLAG, uid, &newiNode))
        return false;
    // Write synlink
    if (!this->WriteFileFromBuf(newiNode, 0, strlen(src), src))
        return false;
    return true;
}
