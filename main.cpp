#include <iostream>
#include <cstring>
#include "VHDController.h"
#include "BSFSParams.h"

using namespace std;

int main()
{
    // Test VHD init
    VHDController vhdc;
    if (!vhdc.Exists())
    {
        cout << "Initializing VHD file " << vhdc.Getfilename() << "...";
        if (vhdc.Create())
            cout << "Done" << endl;
        else
            cout << "Error" << endl;
    }
    else
    {
        cout << "Loading VHD file " << vhdc.Getfilename() << "...";
        if (vhdc.Load())
            cout << "Done" << endl;
        else
            cout << "Error" << endl;
    }
    // Test VHD R/W
    char wbuff[BLOCKSIZE];
    memset(wbuff, 0, BLOCKSIZE);
    strcpy(wbuff, "Hello world");
    vhdc.WriteBlock(0, wbuff);
    vhdc.WriteBlock(BLOCKNUM - 1, wbuff, 5);
    vhdc.WriteBlock(1, (char*)&vhdc, sizeof(VHDController));
    cout << wbuff << endl;
    char rbuff[BLOCKSIZE];
    vhdc.ReadBlock(0, rbuff);
    vhdc.ReadBlock(BLOCKNUM - 1, rbuff);
    cout << rbuff << endl;

    return 0;
}
