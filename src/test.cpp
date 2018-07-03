#include <iostream>
#include <cstring>
#include "VHDController.h"
#include "BSFSParams.h"
#include "test.h"
#include "FSController.h"

using namespace std;

void TestVHDController(VHDController& vhdc)
{
    if (!vhdc.Exists())
    {
        cout << "Initializing VHD file " << vhdc.Getfilename() << "...";
        if (vhdc.Create())
            cout << "Done" << endl;
        else
            cout << "Error" << endl;
    }
    cout << "Loading VHD file " << vhdc.Getfilename() << "...";
    if (vhdc.Load())
        cout << "Done" << endl;
    else
        cout << "Error" << endl;
    // Test VHD R/W
    /*char wbuff[BLOCKSIZE];
    memset(wbuff, 0, BLOCKSIZE);
    strcpy(wbuff, "Hello world");
    vhdc.WriteBlock(0, wbuff);
    vhdc.WriteBlock(BLOCKNUM - 1, wbuff, 5);
    bool a = true;
    vhdc.WriteBlock(1, (char*)&a, sizeof(bool));
    char rbuff[BLOCKSIZE];
    vhdc.ReadBlock(0, rbuff);
    cout << rbuff << endl;
    vhdc.ReadBlock(BLOCKNUM - 1, rbuff);
    cout << rbuff << endl;*/
}

void TestFSController(FSController& fsc)
{
    if (fsc.IsFormat())
        cout << "Already formatted" << endl;
    else
        fsc.Format();
}
