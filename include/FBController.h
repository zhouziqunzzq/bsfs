#ifndef FBCONTROLLER_H
#define FBCONTROLLER_H

#include "VHDController.h"
#include "SuperBlock.h"

using namespace std;

class FBController
{
    public:
        FBController(VHDController& _vhdc, int _sbID, int _minbID, int _maxbID);
        ~FBController();
        bool LoadSuperBLock();
        bool LoadSuperBLock(int blockID);
        bool SaveSuperBlock();
        bool SaveSuperBlock(int blockID);
        bool Recycle(int blockID);
        bool Distribute(int* blockID);

    protected:

    private:
        VHDController& vhdc;
        int superblockID;
        int minblockID;
        int maxblockID;
        SuperBlock superblock;
};

#endif // FBCONTROLLER_H
