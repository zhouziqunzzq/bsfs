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
        bool Format();
        bool IsFormat();
        bool GetBIDByFOff(const iNode& cur, int foff, int* rst);
        bool ReadFileToBuf(const iNode& cur, int start, int len, char* buf);
        bool GetContentInDir(const iNode& curDir, SFD* rst);
        bool FindContentInDir(const SFD* DirSet, const int len, char* name, int* rst);
        bool ParsePath(const iNode& curDir, char* path, bool last, iNode* rst);

    protected:

    private:
        FBController fbc;
        FBController ifbc;
};

#endif // FSCONTROLLER_H
