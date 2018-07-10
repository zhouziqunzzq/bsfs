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
        bool Copy(const iNode& src, iNode& des, char* name, uid_t uid);
        bool Move(iNode& src, iNode& des, char* name);
        bool LinkH(iNode& src, iNode& des, char* name);
        bool LinkS(char* src, iNode& des, char* name, uid_t uid);
        bool ChangeMode(iNode& cur, char mode);
        void GetAbsDir(const iNode& cur, char* dir);
        bool ParsePath(const iNode& curDir, char* path,
                             bool last, iNode* rst, bool followLastSyn = true);
        bool Touch(iNode& curDir, char* name, char mode, int ownerUid, iNode* rst);
        // iNode operation
        bool GetiNodeByID(bid_t id, iNode* rst);
        bool SaveiNodeByID(bid_t id, const iNode& inode);
        // File operation
        bool GetBIDByFOff(const iNode& cur, int foff, bid_t* rst);
        bool ReadFileToBuf(const iNode& cur, int start, int len, char* buf);
        bool AppendBlocksToFile(iNode& cur, int blockCnt);
        bool WriteFileFromBuf(iNode& cur, int start, int len, char* buf);
        bool DeleteFile(iNode& cur);
        // GetCutFile and SaveCutFile is for vim
        // GetCutFile returns cntX(how many lines) and a char array rst[cntX][VIM_MAX_Y]
        // SaveCutFile save a char array buf[cntX][VIM_MAX_Y] into parent dir with
        // given name
        bool GetCutFile(const iNode& cur, char* rst, int* cntX);
        bool SaveCutFile(const iNode& parent, char* name, char* buf, int cntX);
        // Directory operation
        bool GetContentInDir(const iNode& curDir, SFD* rst);
        bool FindContentInDir(const SFD* DirSet, const int len, const char* name, int* rst);
        bool InitDirSFDList(iNode& cur, bid_t parentBid);
        bool CreateRootDir();
        bool CreateSubDir(iNode& curDir, char* name, char mode, uid_t ownerUid, iNode* rst);
        bool DeleteDir(iNode& cur);

    protected:
        //

    private:
        // Block operation
        bool DeleteDirectBlocks(const iNode& cur);
        bool DeleteIndir1Blocks(const iNode& cur);
        bool DeleteIndir2Blocks(const iNode& cur);
        // SFD operation
        bool DeleteSFDEntry(iNode& cur);
        bool AppendSFDEntry(iNode& parent, const SFD& newSFD);
        // File operation
        bool CopyFile(const iNode& src, iNode& des, char* name, uid_t uid);
        // Directory operation
        bool CopyDir(const iNode& src, iNode& des, char* name, uid_t uid);
};

#endif // FSCONTROLLER_H
