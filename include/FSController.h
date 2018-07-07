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
        // File operation
        bool GetBIDByFOff(const iNode& cur, int foff, bid_t* rst);
        bool ReadFileToBuf(const iNode& cur, int start, int len, char* buf);
        bool AppendBlocksToFile(iNode& cur, int blockCnt);
        bool WriteFileFromBuf(iNode& cur, int start, int len, char* buf);
        bool GetiNodeByID(ibid_t id, iNode* rst);
        void GetAbsDir(const iNode& cur, char* dir);
        // Directory operation
        bool GetContentInDir(const iNode& curDir, SFD* rst);
        bool FindContentInDir(const SFD* DirSet, const int len, char* name, int* rst);
        bool ParsePath(const iNode& curDir, char* path, bool last, iNode* rst);
        bool InitDirSFDList(iNode& cur, bid_t parentBid);
        bool CreateRootDir();
        bool CreateSubDir(iNode& curDir, char* dirname, int ownerUid);

    protected:
        //

    private:
        //
};

#endif // FSCONTROLLER_H
