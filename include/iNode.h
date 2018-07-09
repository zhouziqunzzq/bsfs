#ifndef INODE_H_INCLUDED
#define INODE_H_INCLUDED

#include "BSFSParams.h"
#include <cstring>
#include <ctime>

using namespace std;

struct iNode    // sizeof(iNode) = 128
{
    bid_t bid;  // Block ID
    bid_t parent;  // parent iNode block ID
    char name[FILENAME_MAXLEN]; // save filename in iNode for convinience
    char mode;  // d rwx(owner) rwx(public) softlink
    unsigned int nlink; // hard link count (default: 1)
    unsigned short uid; // owner user ID
    unsigned int size;  // file size in bytes
    time_t atime; // access timestamp
    time_t mtime; // modify timestamp
    unsigned int blocks;    // file blocks count
    unsigned short bytes;   // byte count of the last block
    unsigned int data[INODE_DATASIZE];

    iNode()
    {
        memset(&this->bid, 0, sizeof(iNode));
        nlink = 1;
    }
};

#endif // INODE_H_INCLUDED
