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
    /*TestGLP(fsc);
    TestGetBIDByFOff(fsc);
    TestReadFileToBuf(fsc);
    TestAppendBlocksToFile(fsc);
    TestWriteFileFromBuf(fsc);
    TestCreateRootDir(fsc);
    TestCreateSubDir(fsc);*/
    TestParsePath(fsc);

    return 0;
}
