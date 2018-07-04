#ifndef SFD_H_INCLUDED
#define SFD_H_INCLUDED

#include <cstring>
#include "BSFSParams.h"

using namespace std;

struct SFD  // sizeof(SFD) = 32
{
    char name[FILENAME_MAXLEN];
    unsigned short inode;

    SFD()
    {
        memset(this->name, 0, sizeof(SFD));
    }
};

#endif // SFD_H_INCLUDED
