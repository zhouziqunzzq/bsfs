#ifndef INODE_H_INCLUDED
#define INODE_H_INCLUDED

#include "BSFSParams.h"
#include <cstring>

using namespace std;

struct iNode    // sizeof(iNode) = 80
{
    char mode;  // d rwx(owner) rwx(public) softlink
    unsigned int nlink; // hard link count
    unsigned short uid; // user ID
    unsigned int size;  // file size in bytes
    unsigned int atime; // access timestamp
    unsigned int mtime; // modify timestamp
    unsigned int blocks;    // file blocks count
    unsigned short bytes;   // byte count of the last block
    unsigned short parent;  // parent iNode
    unsigned int data[INODE_DATASIZE];

    iNode()
    {
        memset(&this->mode, 0, sizeof(iNode));
    }
};

#endif // INODE_H_INCLUDED
