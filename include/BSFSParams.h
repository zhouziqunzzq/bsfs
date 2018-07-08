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
typedef unsigned short ibid_t;
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
#define MAX_INODE_CNT (iNODEBLOCK_MAX - iNODEBLOCK_MIN + 1)
// SFD
#define FILENAME_MAXLEN 30
// File Mode
#define DIRFLAG 1<<7
#define OWNER_RFLAG 1<<6
#define OWNER_WFLAG 1<<5
#define OWNER_XFLAG 1<<4
#define PUBLIC_RFLAG 1<<3
#define PUBLIC_WFLAG 1<<2
#define PUBLIC_XFLAG 1<<1
#define SYNLINKFLAG 1
#define OWNER_ALLFLAG (OWNER_RFLAG | OWNER_WFLAG | OWNER_XFLAG)
#define PUBLIC_ALLFLAG (PUBLIC_RFLAG | PUBLIC_WFLAG | PUBLIC_XFLAG)
#define DIR_DEFAULT_FLAG (DIRFLAG | OWNER_ALLFLAG | PUBLIC_RFLAG | PUBLIC_XFLAG)
#define FILE_DEFAULT_FLAG (OWNER_RFLAG | OWNER_WFLAG | PUBLIC_RFLAG)
// User
#define ROOT_UID 1
#define MAX_UNAME_LEN 32
// CLI
#define MAX_CMD_LEN 1024
// Process
#define MAX_OPEN_CNT_P 128
#define MAX_OPEN_CNT_F 128
#define MAX_PROCESS_CNT 65535
// Other
#define WELCOMEMSG "Booting BSFS..."
#define MAXFOLLOWLINK 40
#define HOSTNAME "yukiyukiyu"

#endif // BSFSPARAMS_H_INCLUDED

/*
Block 0: MBR
#0 byte: Format flag (not format: 00H, format: 01H)
#1... bytes: Welcome message ("Booting BSFS...")

Block1: Super Block
*/
