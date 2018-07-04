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
#define iNODEBLOCK_MIN 3
#define iNODEBLOCK_MAX 1026
#define DATABLOCK_MIN 1027
#define DATABLOCK_MAX BLOCKNUM - 1
// GLP(Grouped Linked Pointers)
#define GROUPSIZE (BLOCKSIZE) / sizeof(int) - 1
// iNode
#define INODE_DATASIZE 12
#define INODE_DIRECT_CNT 10
#define INODE_1INDIR_CNT 1
#define INODE_2INDIR_CNT 1
#define INODE_DIRECT_MAX INODE_DIRECT_CNT - 1
#define INODE_1INDIR_MAX INODE_DIRECT_MAX + INODE_1INDIR_CNT
#define INODE_2INDIR_MAX INODE_1INDIR_MAX + INODE_2INDIR_CNT
// SFD
#define FILENAME_MAXLEN 30
// Other
#define WELCOMEMSG "Booting BSFS..."

#endif // BSFSPARAMS_H_INCLUDED

/*
Block 0: MBR
#0 byte: Format flag (not format: 00H, format: 01H)
#1... bytes: Welcome message ("Booting BSFS...")

Block1: Super Block
*/
