// McGill Fall 2023 - COMP 310 - Operating Systems
// Elliot Forcier-Poirier
// 260989602

/* Super Block */
#define BLOCKSIZE_ 1024  // large block size means one structure per block
#define SB_MAGIC_ 888    // magic number for superblock
#define SB_BLOCK_ 0      // address of superblock
#define NUM_BLOCKS_ 1024 // free data block bitmap restriction (max is more but playing it it safe)

/* I-node Table*/
#define ICACHE_BLOCK_START_ 1
#define ICACHE_BLOCK_END_ 31 // actual number would be 18 = (15 x 4) x 256 / 1024, but wanted to be safe
#define ICACHE_NUM_BLOCKS ICACHE_BLOCK_END_ - ICACHE_BLOCK_START_ + 1
#define NUM_INODES_ 256 // 101 inodes means 7 blocks in the inode table

#define FBM_BLOCK_ 1023 // free data block bitmap block address

#define MAX_FILES_ 255 // 96 files max
#define MAX_FILE_NAME_ 16
#define MAX_FILE_EXT_ 3
#define DIR_INODE_ 100
#define NUM_DIR_DATABLOCKS_ 12
#define NUM_PTRS_INDIR_ BLOCKSIZE_ / 6                                 // pts are 4 byte ints but I am being cautious because of packing & padding
#define MAX_FILE_DATABLOCKS_ = (NUM_DIR_DATABLOCKS_ + NUM_PTRS_INDIR_) // 12 + 170 = 182 max data blocks per file
#define MAX_FILE_SIZE_ MAX_FILE_DATABLOCKS_ *BLOCKSIZE_
#define MAX_ICACHE_SIZE_ 8192 //[(NUM_DIR_DATABLOCKS_ + 3)*4] * NUM_INODES_] raised to the nearest power of 2
                              // haven't figured out how to do this dynamically pre compilation
#define ICACHE_BLOCKCOUNT_ MAX_ICACHE_SIZE_ / BLOCKSIZE_
#define MAX_DIR_SIZE_ 4096 //(MAX_FILE_NAME_+MAX_FILE_EXT_+10) * MAX_FILES_ + 4 raised to next power of 2
#define DIR_BLOCKCOUNT_ MAX_DIR_SIZE_ / BLOCKSIZE_

/* Data Blocks */
#define FIRST_DATABLOCK_ 32                             // first data block address
#define LAST_DATABLOCK_ 1022                            // last data block address
#define DATA_BLOCKS_AVAIL_ LAST_DATABLOCK_ - FBM_BLOCK_ // 3551

#include <stdlib.h>
#include <string.h>
#include <stdio.h>