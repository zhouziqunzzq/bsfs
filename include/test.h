#ifndef TEST_H_INCLUDED
#define TEST_H_INCLUDED

#include "VHDController.h"
#include "FSController.h"

void TestVHDController(VHDController& vhdc);
void TestFSController(FSController& fsc);
void TestGLP(FSController& fsc);
void TestGetBIDByFOff(FSController& fsc);
void TestReadFileToBuf(FSController& fsc);
void TestAppendBlocksToFile(FSController &fsc);
void TestWriteFileFromBuf(FSController &fsc);
void TestCreateRootDir(FSController& fsc);
void TestCreateSubDir(FSController& fsc);
void TestParsePath(FSController& fsc);
void TestParsePath(FSController& fsc);

#endif // TEST_H_INCLUDED
