#ifndef TEST_H_INCLUDED
#define TEST_H_INCLUDED

#include "VHDController.h"
#include "FSController.h"

void TestVHDController(VHDController& vhdc);
void TestFSController(FSController& fsc);
void TestGetBIDByFOff(FSController& fsc);
void TestReadFileToBuf(FSController& fsc);

#endif // TEST_H_INCLUDED
