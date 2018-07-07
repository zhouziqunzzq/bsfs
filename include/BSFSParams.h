#ifndef BSFSPARAMS_H_INCLUDED
#define BSFSPARAMS_H_INCLUDED

// VHD
#define VHDSIZE 1024*1024*64    // 64MB
#define VHDFILENAME "data.img"
// Block
#define BLOCKSIZE 1024*1    // 1KB
#define BLOCKNUM (VHDSIZE) / (BLOCKSIZE)
#define MBRBLOCK 0
#define SUPERBLOCK 1
#define iSUPERBLOCK 2
#define ROOTDIRiNODE 3
#define iNODEBLOCK_MIN 4
#define iNODEBLOCK_MAX 1026
#define DATABLOCK_MIN 1027
#define DATABLOCK_MAX BLOCKNUM - 1
typedef unsigned int bid_t;
// GLP(Grouped Linked Pointers)
#define GROUPSIZE (BLOCKSIZE) / sizeof(bid_t) - 1
// iNode
#define INODE_DATASIZE 12
#define INODE_DIRECT_CNT 10
#define INODE_INDIR1_CNT 1
#define INODE_INDIR2_CNT 1
#define INODE_DIRECT_MAX INODE_DIRECT_CNT - 1
#define INODE_INDIR1_MAX INODE_DIRECT_MAX + INODE_INDIR1_CNT
#define INODE_INDIR2_MAX INODE_INDIR1_MAX + INODE_INDIR2_CNT
#define DIRECT_BLOCK_CNT INODE_DIRECT_CNT
#define INDIR1_BLOCK_CNT (BLOCKSIZE) / sizeof(bid_t)
#define INDIR2_BLOCK_CNT ((BLOCKSIZE) * (BLOCKSIZE)) / (sizeof(bid_t) * sizeof(bid_t))
#define MAX_BLOCK_CNT (DIRECT_BLOCK_CNT + INDIR1_BLOCK_CNT + INDIR2_BLOCK_CNT -1 - (BLOCKSIZE / sizeof(bid_t)))
#define ROOT_INODE iNODEBLOCK_MIN
// SFD
#define FILENAME_MAXLEN 30
// File Mode
#define DIRFLAG 1<<7
#define SYNLINKFLAG 1
// Other
#define WELCOMEMSG "Booting BSFS..."
#define MAXFOLLOWLINK 40

#endif // BSFSPARAMS_H_INCLUDED

/*
Block 0: MBR
#0 byte: Format flag (not format: 00H, format: 01H)
#1... bytes: Welcome message ("Booting BSFS...")

Block1: Super Block
*/
