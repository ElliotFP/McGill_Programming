// McGill Fall 2023 - COMP 310 - Operating Systems
// Elliot Forcier-Poirier
// 260989602

#include "disk_emu.h"
#include "sfs_api.h"
#include "constants.h"
#include "mem_data_structures.h"
#include "disk_data_structures.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int errCode = 0; // Global error code
char sfs_disk_name[] = "BananaDisk.disk";
// int icacheBlocknums[ICACHE_NUM_BLOCKS];
icache *ic;
superblock *sb;
FDB *fd;
directory *dir;
FDT *ft;

/* ------- */
/* mksfs() */
/* ------- */
/* This function creates a new file system or loads an existing one */

void init_icache()
{
    // initialize icache
    int x;
    ic = i_initCache();

    // allocate memory for icache
    char *buffer = (char *)malloc(sizeof(ic));
    memcpy(buffer, ic, sizeof(*ic));

    // write icache to disk
    write_blocks(ICACHE_BLOCK_START_, ICACHE_NUM_BLOCKS, buffer);
    free(buffer);
}

// void init_freebitmap()
// {
//     // initialize free data blocks
//     fd = FDB_init();
//     char *buffer = (char *)malloc(sizeof(fd));
//     memcpy(buffer, fd, sizeof(fd));
//     write_blocks(FDB_BLOCK_, 1, buffer);
//     free(buffer);
// }

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

        // initialize free data blocks
        fd = FDB_init();
        write_blocks(FBM_BLOCK_, 1, FDB_init()); // write free data block free bitmap to disk

        init_icache();
        // init_freebitmap();
        dir = d_initDir();
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

int sfs_fopen(char *p)
{
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
