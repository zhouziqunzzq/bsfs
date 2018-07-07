#ifndef FBCONTROLLER_H
#define FBCONTROLLER_H

#include "VHDController.h"
#include "SuperBlock.h"

using namespace std;

class FBController
{
    public:
        FBController(VHDController& _vhdc, bid_t _sbID, bid_t _minbID, bid_t _maxbID);
        ~FBController();
        bool LoadSuperBLock();
        bool LoadSuperBLock(bid_t blockID);
        bool SaveSuperBlock();
        bool SaveSuperBlock(bid_t blockID);
        bool Recycle(bid_t blockID);
        bool Distribute(bid_t* blockID);
        void SetFullFlag();

    protected:

    private:
        VHDController& vhdc;
        int superblockID;
        int minblockID;
        int maxblockID;
        SuperBlock superblock;
};

#endif // FBCONTROLLER_H
