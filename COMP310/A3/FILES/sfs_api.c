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
        write_blocks(SB_BLOCK_, 1, s_init()); // write superblock to disk

        ic = i_initCache(); // initialize inode cache
        write_blocks(ICACHE_BLOCK_START_, ICACHE_NUM_BLOCKS, ic);

        fbm = b_init(DATA_BLOCKS_AVAIL_); // initialize free data block free bitmap
        write_blocks(FBM_BLOCK_, 1, fbm);

        dir = get_dir(); // get pointer to directory
    }
    else // load existing file system
    {
        errCode = init_disk(sfs_disk_name, BLOCKSIZE_, NUM_BLOCKS_);
        if (errCode == -1)
        {
            printf("Error initializing disk\n");
            return;
        }
        ic = get_icache(); // get pointer to inode cache
        fbm = getBitmap(); // get pointer to free data block bitmap
        dir = get_dir();   // get pointer to directory
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
    // check if file exists
    int i;

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
    int f = sfs_fopen("some_name.txt");
    return 0;
}
