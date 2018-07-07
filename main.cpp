#include <iostream>
#include <cstring>
#include <ctime>
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
    TestGetBIDByFOff(fsc);
    TestReadFileToBuf(fsc);
    TestAppendBlocksToFile(fsc);
    TestWriteFileFromBuf(fsc);

    return 0;
}
