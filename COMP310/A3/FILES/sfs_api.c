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
        sb = s_init();                  // initialize superblock
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
    ft = f_init(); // initialize file descriptor table
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
        if (inode_num == -1)
        {
            printf("No free inodes\n");
            return -1;
        }

        // initialize inode
        inode *i = init_inode(inode_num);
        ic->i[inode_num] = *i;

        d_addEntry(name, inode_num); // add new entry to directory

        free(i); // free inode

        // update disk
        write_blocks(ICACHE_BLOCK_START_, ICACHE_NUM_BLOCKS, ic); // write inode cache to disk
        write_blocks(FIRST_DATABLOCK_, 1, dir);                   // write directory to disk
    }
    else // file found
    {
        printf("File found\n");
    }

    int fd = f_createEntry(inode_num); // get free file descriptor table entry
    if (fd == -1)
    {
        printf("No free file descriptors\n");
        return -1;
    }
    printf("File opened\n");
    return fd;
}

/* ------------ */
/* sfs_fclose() */
/* ------------ */
/* This function closes a file */
int sfs_fclose(int fd)
{
    if (ft->f[fd].active == 0)
    {
        printf("File descriptor not active\n");
        return -1;
    }

    f_deactivate(fd); // deactivate file descriptor

    return 0;
}

/* ------------ */
/* sfs_fwrite() */
/* ------------ */
/* This function writes to a file */

int write_to_block(int block_num, int rw, const char *buffer) // helper function to write to a specific block given the current rw in that block
{
    if (block_num == -1)
    {
        printf("No free blocks\n");
        return -1;
    }
    int length = strlen(buffer);

    char *block = (char *)malloc(BLOCKSIZE_);
    read_blocks(block_num + FIRST_DATABLOCK_, 1, block); // read block from disk and offset so that it starts at the first data block

    int i = 0;
    while (rw + i < BLOCKSIZE_ && i < length) // write buffer to block
    {
        block[rw + i] = buffer[i];
        i++;
    }
    printf("length: %d\n", length);
    printf("rw +i: %d\n", i);
    printf("buffer char at i: %c\n", buffer[i]);
    write_blocks(block_num + FIRST_DATABLOCK_, 1, block); // write block to disk and offset so that it starts at the first data block
    rw += i;                                              // increment rw in block
    return rw;
}

int sfs_fwrite(int fd, const char *buffer, int length)
{
    printf("Writing %d bytes to file\n", length);
    printf("rw %d\n", ft->f[fd].rw);

    if (ft->f[fd].active == 0)
    {
        printf("File descriptor not active\n");
        return -1;
    }

    int rw = ft->f[fd].rw; // current rw in file
    int inode_num = ft->f[fd].inode;

    if (rw == 0) // initialize block pointer if rw is 0
    {
        ic->i[inode_num].pointers[0] = b_getfreebit(); // get free block

        if (ic->i[inode_num].pointers[0] == -1)
        {
            return -1;
        }
    }

    int new_length = length + rw;      // new length of file
    int block_num = rw / BLOCKSIZE_;   // current block number
    int rw_in_block = rw % BLOCKSIZE_; // current rw in block
    printf("ftrw: %d\n", rw);
    printf("rw_in_block: %d\n", rw_in_block);
    printf("newlength: %d\n", new_length);

    while (rw < new_length) // write buffer to file
    {
        rw_in_block = write_to_block(ic->i[inode_num].pointers[block_num], rw_in_block, buffer); // write to block

        if (rw == -1)
        {
            printf("No free blocks\n");
            return -1;
        }
        else if (rw_in_block == BLOCKSIZE_) // if block is full, get new block
        {
            printf("Block full\n");
            block_num++; // increment block number

            if (block_num >= NUM_DIR_DATABLOCKS_) // if block number is out of range we have to use an index block
            {
                printf("Index block\n");
                return -1;
            }

            ic->i[inode_num].pointers[block_num] = b_getfreebit(); // get free block
            rw_in_block = 0;                                       // reset rw in block
            buffer += BLOCKSIZE_ - rw % BLOCKSIZE_;                // increment buffer

            if (ic->i[inode_num].pointers[block_num] == -1)
            {
                printf("No free blocks\n");
                return -1;
            }
        }

        rw = block_num * BLOCKSIZE_ + rw_in_block; // increment rw in file
        printf("ftrw: %d\n", rw);
    }

    printf("rw: %d\n", rw);
    ft->f[fd].rw = rw;
    ic->i[inode_num].size = rw; // update file size
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

    int f1 = sfs_fopen("some_name.txt");
    // int f2 = sfs_fopen("some_name2.txt");
    char *buffer1 = "Hello World!";
    char *buffer2 = "Goodbye World!";
    sfs_fwrite(f1, buffer1, strlen(buffer1));
    sfs_fwrite(f1, buffer2, strlen(buffer2));
    sfs_fclose(f1);

    // // print it from the disk
    int i;
    char *buffer4 = (void *)malloc(BLOCKSIZE_);
    read_blocks(FIRST_DATABLOCK_ + 1, 1, buffer4);
    for (i = 0; i < 1024; i++)
    {
        printf("%c", buffer4[i]);
    }

    // // // print it from the disk
    // int i;
    // char *buffer4 = (void *)malloc(BLOCKSIZE_);
    // read_blocks(ic->i[0].pointers[0] + FIRST_DATABLOCK_, 1, buffer4);
    // for (i = 0; i < 1024; i++)
    // {
    //     printf("%c", buffer4[i]);
    // }

    // FOR LOOP TO SEE WHAT HAPPENS WHEN WE GO BEYOND A BLOCK
    int f = sfs_fopen("some_name.txt");
    char *buffer6 = "notre pere qui etes aux cieu, que votre nom soit sanctifie, que votre regne vienn, que votre volonte soit faite sur la terre comme au ciel, donnez-nous aujourd'hui notre pain quotidien, pardonnez-nous nos offenses comme nous pardonnons aussi a ceux qui nous ont offensés, et ne nous soumettez pas a la tentation, mais delivrez-nous du mal, amen.";
    sfs_fwrite(f, buffer6, strlen(buffer6));
    sfs_fwrite(f, buffer6, strlen(buffer6));
    sfs_fwrite(f, buffer6, strlen(buffer6));
    sfs_fwrite(f, buffer6, strlen(buffer6));
    sfs_fwrite(f, buffer6, strlen(buffer6));
    sfs_fwrite(f, buffer6, strlen(buffer6));
    sfs_fwrite(f, buffer6, strlen(buffer6));
    sfs_fclose(f);

    // print it from the disk
    char *buffer5 = (void *)malloc(BLOCKSIZE_);
    read_blocks(ic->i[0].pointers[0] + FIRST_DATABLOCK_, 1, buffer5);
    for (i = 0; i < 1024; i++)
    {
        printf("%c", buffer5[i]);
    }
    printf("\n ____________ \n");

    // char *buffer5 = (void *)malloc(BLOCKSIZE_);
    read_blocks(ic->i[0].pointers[1] + FIRST_DATABLOCK_, 1, buffer5);
    for (i = 0; i < 1024; i++)
    {
        printf("%c", buffer5[i]);
    }
    printf("\n ____________ \n");
    char *buffer7 = (void *)malloc(BLOCKSIZE_);
    read_blocks(ic->i[0].pointers[2] + FIRST_DATABLOCK_, 1, buffer7);
    for (i = 0; i < 1024; i++)
    {
        printf("%c", buffer7[i]);
    }
    // printf("\n");
    // char *buffer8 = (void *)malloc(BLOCKSIZE_);
    // read_blocks(ic->i[0].pointers[3] + FIRST_DATABLOCK_, 1, buffer8);
    // for (i = 0; i < 1024; i++)
    // {
    //     printf("%c", buffer8[i]);
    // }

    // char *buffer4 = (void *)malloc(BLOCKSIZE_);
    // read_blocks(ic->i[1].pointers[0] + FIRST_DATABLOCK_, 1, buffer4);
    // int *buffer5 = (void *)malloc(BLOCKSIZE_);
    // read_blocks(FIRST_DATABLOCK_, 1, buffer5);

    // int i;
    // for (i = 0; i < 100; i++)
    // {
    //     printf("%c", buffer4[i]);
    //     // printf("%d ", buffer5[i]);
    // }

    return 0;
}
