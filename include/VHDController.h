#ifndef VHDCONTROLLER_H
#define VHDCONTROLLER_H
#include <string>
#include <fstream>
#include "BSFSParams.h"

using namespace std;

class VHDController
{
public:
    VHDController();
    ~VHDController();

    int Getsize()
    {
        return size;
    }
    void Setsize(int val)
    {
        size = val;
    }
    string Getfilename()
    {
        return filename;
    }
    void Setfilename(string val)
    {
        filename = val;
    }

    bool Create();    // Init a Virtual HD file with given filename and size
    bool Exists();  // Check the existence of VHD file
    bool Load();    // Load VHD file
    bool ReadBlock(bid_t blockID, char *buffer,
                   int len = BLOCKSIZE);  // Read one block to buffer
    bool WriteBlock(bid_t blockID, char *buffer,
                    int len = BLOCKSIZE);    // Write one block from buffer

protected:

private:
    int size;
    string filename;
    fstream _file;
};

#endif // VHDCONTROLLER_H
