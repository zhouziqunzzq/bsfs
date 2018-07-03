#include <iostream>
#include <cstring>
#include "test.h"
#include "BSFSParams.h"
#include "VHDController.h"
#include "FSController.h"

using namespace std;

int main()
{
    VHDController vhdc;
    TestVHDController(vhdc);

    FSController fsc(vhdc);
    TestFSController(fsc);

    return 0;
}
