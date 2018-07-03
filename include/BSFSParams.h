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
// GLP(Grouped Linked Pointers)
#define GROUPSIZE (BLOCKSIZE) / sizeof(int) - 4
// Other
#define WELCOMEMSG "Booting BSFS..."

#endif // BSFSPARAMS_H_INCLUDED

/*
Block 0: MBR
#0 byte: Format flag (not format: 00H, format: 01H)
#1... bytes: Welcome message ("Booting BSFS...")

Block1: Super Block
*/
