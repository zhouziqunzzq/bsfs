#include<iostream>
#include "FBController.h"
#include "BSFSParams.h"
#include "VHDController.h"

using namespace std;

FBController::FBController(VHDController& _vhdc, bid_t _sbID, bid_t _minbID, bid_t _maxbID) :
    vhdc(_vhdc), superblockID(_sbID), minblockID(_minbID), maxblockID(_maxbID)
{
    this->LoadSuperBLock();
    return;
}

FBController::~FBController()
{
    this->SaveSuperBlock();
    return;
}

bool FBController::LoadSuperBLock()
{
    return this->vhdc.ReadBlock(this->superblockID,
                                (char*)&this->superblock, sizeof(SuperBlock));
}

bool FBController::LoadSuperBLock(bid_t blockID)
{
    return this->vhdc.ReadBlock(blockID,
                                (char*)&this->superblock, sizeof(SuperBlock));
}

bool FBController::SaveSuperBlock()
{
    return this->vhdc.WriteBlock(this->superblockID,
                                 (char*)&this->superblock, sizeof(SuperBlock));
}

bool FBController::SaveSuperBlock(bid_t blockID)
{
    return this->vhdc.WriteBlock(blockID,
                                 (char*)&this->superblock, sizeof(SuperBlock));
}

bool FBController::Recycle(bid_t blockID)
{
    if (this->superblock.cnt == GROUPSIZE)
    {
        if(!this->SaveSuperBlock(blockID))
            return false;
        memset((char*)&this->superblock, 0, sizeof(SuperBlock));
        this->superblock.cnt = 1;
    }
    else this->superblock.cnt++;
    this->superblock.freeStack[this->superblock.cnt-1] = blockID;
    return true;
}

bool FBController::Distribute(bid_t* blockID)
{
    if(this->superblock.freeStack[this->superblock.cnt-1] == 0) return false;
    *blockID = this->superblock.freeStack[this->superblock.cnt-1];
    if(this->superblock.cnt == 1)
    {
        if(!this->LoadSuperBLock(*blockID))
            return false;
    }
    else this->superblock.cnt--;
    return true;
}

void FBController::SetFullFlag()
{
    this->superblock.freeStack[0] = 0;
    this->superblock.cnt = 1;
}
