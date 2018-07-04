#include<iostream>
#include "FBController.h"
#include "BSFSParams.h"
#include "VHDController.h"

using namespace std;

FBController::FBController(VHDController& _vhdc, int _sbID, int _minbID, int _maxbID) :
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

bool FBController::SaveSuperBlock()
{
    return this->vhdc.WriteBlock(this->superblockID,
                                 (char*)&this->superblock, sizeof(SuperBlock));
}

bool FBController::Recycle(int blockID)
{
    if (this->superblock.cnt == GROUPSIZE)
    {
        if(!this->vhdc.WriteBlock(blockID, (char*)&this->superblock, sizeof(SuperBlock)))
            return false;
        memset((char*)&this->superblock, 0, sizeof(SuperBlock));
        this->superblock.cnt = 1;
    }
    else this->superblock.cnt++;
    this->superblock.freeStack[this->superblock.cnt-1] = blockID;
    return true;
}

bool FBController::Distribute(int* blockID)
{
    if(this->superblock.freeStack[this->superblock.cnt-1] == 0) return false;
    *blockID = this->superblock.freeStack[this->superblock.cnt-1];
    if(this->superblock.cnt == 1)
    {
        if(!this->vhdc.ReadBlock(*blockID, (char*)&this->superblock, sizeof(SuperBlock)))
            return false;
    }
    return true;
}
