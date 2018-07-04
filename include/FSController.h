#ifndef FSCONTROLLER_H
#define FSCONTROLLER_H

#include "VHDController.h"
#include "FBController.h"
#include "iNode.h"

using namespace std;

class FSController
{
    public:
        FSController(VHDController& _vhdc);
        bool Format();
        bool IsFormat();
        bool GetBIDByFOff(const iNode& cur, int foff, int* rst);
        bool ReadFileToBuf(const iNode& cur, int start, int len, char* buf);

    protected:

    private:
        VHDController& vhdc;
        FBController fbc;
        FBController ifbc;
};

#endif // FSCONTROLLER_H
