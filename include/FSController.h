#ifndef FSCONTROLLER_H
#define FSCONTROLLER_H

#include "VHDController.h"

using namespace std;

class FSController
{
    public:
        FSController(VHDController& _vhdc);
        bool Format();
        bool IsFormat();

    protected:

    private:
        VHDController& vhdc;
};

#endif // FSCONTROLLER_H
