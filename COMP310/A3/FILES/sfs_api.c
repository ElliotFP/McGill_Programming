// McGill Fall 2023 - COMP 310 - Operating Systems
// Elliot Forcier-Poirier
// 260989602

#include "disk_emu.h"
#include "sfs_api.h"
#include "constants.h"
#include "structures.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int errCode = 0; // Global error code
char sfs_disk_name[] = "BananaDisk.disk";
// int icacheBlocknums[ICACHE_NUM_BLOCKS];
icache *ic;
superblock *sb;
Bitmap *fbm;
directory *dir;
FDT *ft;

/* ------- */
/* mksfs() */
/* ------- */
/* This function creates a new file system or loads an existing one */

void mksfs(int fresh)
{
    if (fresh) // create new file system
    {
        printf("Creating new file system\n");
        errCode = init_fresh_disk(sfs_disk_name, BLOCKSIZE_, NUM_BLOCKS_); // initialize disk
        if (errCode == -1)
        {
            printf("Error initializing disk\n");
            return;
        }

        //  Initialize all disk data structures
        sb = s_init(sb);                // initialize superblock
        write_blocks(SB_BLOCK_, 1, sb); // write superblock to disk

        ic = i_initCache();                                       // initialize inode cache
        write_blocks(ICACHE_BLOCK_START_, ICACHE_NUM_BLOCKS, ic); // write inode cache to disk

        fbm = b_init(DATA_BLOCKS_AVAIL_); // initialize free block bitmap
        write_blocks(FBM_BLOCK_, 1, fbm); // write free block bitmap to disk

        dir = d_init(NUM_INODES_ - 1);          // initialize directory
        write_blocks(FIRST_DATABLOCK_, 1, dir); // write directory to disk
    }
    else // load existing file system
    {
        errCode = init_disk(sfs_disk_name, BLOCKSIZE_, NUM_BLOCKS_);
        if (errCode == -1)
        {
            printf("Error initializing disk\n");
            return;
        }
    }
}

/* --------------------- */
/* sfs_getnextfilename() */
/* --------------------- */
/* This function returns the next filename in the directory */

int sfs_getnextfilename(char *p)
{
    return 0;
}

/* ----------------- */
/* sfs_getfilesize() */
/* ----------------- */
/* This function returns the size of a file */

int sfs_getfilesize(const char *p)
{
    return 0;
}

/* ----------- */
/* sfs_fopen() */
/* ----------- */
/* This function opens a file */

int sfs_fopen(char *name)
{
    int inode_num = d_getFile(name); // get inode number of file
    if (inode_num == -1)
    {
        printf("File not found\n");

        // create new file
        inode_num = get_free_inode(); // get free inode
        printf("Free inode: %d\n", inode_num);
        if (inode_num == -1)
        {
            printf("No free inodes\n");
            return -1;
        }

        // initialize inode
        inode *i = init_inode(inode_num);
        ic->i[inode_num] = *i;

        printf("Inode initialized\n");
        d_addEntry(name, inode_num); // add new entry to directory
        printf("Directory entry added\n");

        // create new file descriptor
        FDTentry *fd = f_createEntry(inode_num); // get free file descriptor table entry
        ft->f[inode_num] = *fd;                  // add file descriptor to file table
    }
    ft = f_init(ft); // initialize file descriptor table

    return 0;
}

/* ------------ */
/* sfs_fclose() */
/* ------------ */
/* This function closes a file */
int sfs_fclose(int fd)
{
    return 0;
}

/* ------------ */
/* sfs_fwrite() */
/* ------------ */
/* This function writes to a file */

int sfs_fwrite(int a, const char *p, int b)
{
    return 0;
}

/* ----------- */
/* sfs_fread() */
/* ----------- */
/* This function reads from a file */

int sfs_fread(int a, char *p, int b)
{
    return 0;
}

/* ----------- */
/* sfs_fseek() */
/* ----------- */
/* This function seeks to a position in a file */

int sfs_fseek(int a, int b)
{
    return 0;
}

/* ------------ */
/* sfs_remove() */
/* ------------ */
/* This function removes a file */

int sfs_remove(char *p)
{
    return 0;
}

/* ------ */
/* main() */
/* ------ */

int main()
{
    mksfs(1);
    // int f = sfs_fopen("some_name.txt");

    return 0;
}
