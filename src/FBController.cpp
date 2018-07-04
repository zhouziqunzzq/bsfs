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
    return true;
}

bool FBController::Distribute(int* blockID)
{
    return true;
}
