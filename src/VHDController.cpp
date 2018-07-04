#include "VHDController.h"
#include "BSFSParams.h"
#include <fstream>
#include <cstring>

using namespace std;

VHDController::VHDController()
{
    this->size = VHDSIZE;
    this->filename = VHDFILENAME;
}

VHDController::~VHDController()
{
    this->_file.close();
}

bool VHDController::Create()
{
    // Init outstream
    ofstream vhdFile;
    vhdFile.open(this->filename.c_str(), ios::out | ios::binary);
    if (!vhdFile)
        return false;
    // Init memory buffer
    char *buff = new char[this->size];
    memset(buff, 0, this->size);
    // Write out
    vhdFile.write(buff, this->size);
    if (!vhdFile)
        return false;
    vhdFile.close();
    delete[] buff;
    return true;
}

bool VHDController::Exists()
{
    ifstream f(this->filename.c_str());
    return f.good();
}

bool VHDController::Load()
{
    this->_file.open(this->filename.c_str(), ios::in | ios::out | ios::binary);
    return this->_file.good();
}

bool VHDController::ReadBlock(int blockID, char *buffer, int len)
{
    if (blockID < BLOCKNUM && len <= BLOCKSIZE)
    {
        this->_file.seekg(blockID * BLOCKSIZE);
        this->_file.read(buffer, len);
        if (!this->_file)
            return false;
        else
            return true;
    }
    else
    {
        return false;
    }
}

bool VHDController::WriteBlock(int blockID, char *buffer, int len)
{
    if (blockID < BLOCKNUM && len <= BLOCKSIZE)
    {
        this->_file.seekp(blockID * BLOCKSIZE);
        this->_file.write(buffer, len);
        if (!this->_file)
            return false;
        else
            return true;
    }
    else
    {
        return false;
    }
}
