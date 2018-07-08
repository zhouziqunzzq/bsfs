#ifndef FSCONTROLLER_H
#define FSCONTROLLER_H

#include "VHDController.h"
#include "FBController.h"
#include "iNode.h"
#include "SFD.h"

using namespace std;

class FSController
{
    public:
        FSController(VHDController& _vhdc);
        VHDController& vhdc;
        FBController fbc;
        FBController ifbc;
        // Format
        bool Format();
        bool IsFormat();
        // General operation
        bool Copy(const iNode& src, const iNode& des, char* name);
        bool Move(const iNode& src, const iNode& des, char* name);
        bool ChangeMode(const iNode& cur, char mode);
        void GetAbsDir(const iNode& cur, char* dir);
        bool ParsePath(const iNode& curDir, char* path, bool last, iNode* rst);
        bool Touch(iNode& curDir, char* name, char mode, int ownerUid, iNode* rst);
        // iNode operation
        bool GetiNodeByID(bid_t id, iNode* rst);
        bool SaveiNodeByID(bid_t id, const iNode& inode);
        // File operation
        bool GetBIDByFOff(const iNode& cur, int foff, bid_t* rst);
        bool ReadFileToBuf(const iNode& cur, int start, int len, char* buf);
        bool AppendBlocksToFile(iNode& cur, int blockCnt);
        bool WriteFileFromBuf(iNode& cur, int start, int len, char* buf);
        bool DeleteFile(const iNode& cur);
        // Directory operation
        bool GetContentInDir(const iNode& curDir, SFD* rst);
        bool FindContentInDir(const SFD* DirSet, const int len, char* name, int* rst);
        bool InitDirSFDList(iNode& cur, bid_t parentBid);
        bool CreateRootDir();
        bool CreateSubDir(iNode& curDir, char* name, char mode, int ownerUid);
        bool DeleteDir(const iNode& cur);

    protected:
        //

    private:
        bool DeleteDirectBlocks(const iNode& cur);
        bool DeleteIndir1Blocks(const iNode& cur);
        bool DeleteIndir2Blocks(const iNode& cur);
        bool DeleteSFDEntry(const iNode& cur);
};

#endif // FSCONTROLLER_H
