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

    char *block = (char *)calloc(1, BLOCKSIZE_);
    read_blocks(block_num + FIRST_DATABLOCK_, 1, block); // read block from disk and offset so that it starts at the first data block

    int i = 0;
    while (rw + i < BLOCKSIZE_ && i < length) // write buffer to block
    {
        block[rw + i] = buffer[i];
        i++;
    }
    write_blocks(block_num + FIRST_DATABLOCK_, 1, block); // write block to disk and offset so that it starts at the first data block

    free(block); // free block
    rw += i;     // increment rw in block
    return rw;
}

int sfs_fwrite(int fd, const char *buffer, int length)
{
    printf("Writing %d bytes to file\n", length);

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

    int size_of_data = sizeof(buffer); // size of data in file
    int new_length = length + rw;      // new length of file
    int block_num = rw / BLOCKSIZE_;   // current block number
    int rw_in_block = rw % BLOCKSIZE_; // current rw in block
    int bytes_written = 0;             // number of bytes written
    int start = rw;                    // start of write

    while (rw < new_length && bytes_written < size_of_data) // write buffer to file
    {
        printf("Writing to block %d, rw in block %d\n", rw_in_block, block_num);
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
        bytes_written = rw - start;                // increment buffer
    }

    ft->f[fd].rw = rw;          // update rw in file descriptor
    ic->i[inode_num].size = rw; // update file size

    return length;
}

/* ----------- */
/* sfs_fread() */
/* ----------- */
/* This function reads from a file */

int read_from_block(int block_num, int rw, char *buffer, int buffer_size) // helper function to read from a specific block given the current rw in that block
{
    if (block_num == -1)
    {
        printf("incorrect block address\n");
        return -1;
    }
    if (buffer_size == 0)
    {
        printf("File is empty\n");
        return -1;
    }

    int rw_in_block = rw % BLOCKSIZE_;              // current rw in block
    int length_in_block = BLOCKSIZE_ - rw_in_block; // length of file in block
    char *block = (char *)calloc(1, BLOCKSIZE_);
    read_blocks(block_num + FIRST_DATABLOCK_, 1, block); // read block from disk and offset so that it starts at the first data block

    int i = 0;
    while (rw_in_block + i < BLOCKSIZE_ && rw_in_block + i < length_in_block && rw < buffer_size) // read block to buffer
    {
        buffer[rw] = block[rw_in_block + i];
        rw++;
        i++;
    }

    free(block); // free block
    return rw;
}

int sfs_fread(int fd, char *buffer, int buffer_size)
{
    printf("Reading %d bytes from file, starting at %d\n", buffer_size, ft->f[fd].rw);
    if (ft->f[fd].active == 0)
    {
        printf("File descriptor not active\n");
        return -1;
    }
    if (buffer_size < 0)
    {
        printf("Length must be positive or file is empty\n");
        return -1;
    }

    int inode_num = ft->f[fd].inode;       // inode number of file
    int rw = ft->f[fd].rw;                 // current rw in file
    int file_size = ic->i[inode_num].size; // size of file
    int bytes_read = 0;                    // number of bytes read
    int block_num = rw / BLOCKSIZE_;       // current block number
    int rw_in_block = rw % BLOCKSIZE_;     // current rw in block
    int start = rw;                        // start of read

    if (rw == file_size)
    {
        printf("File is empty or pointer is at end of file\n");
        return -1;
    }

    while (rw < file_size && bytes_read < buffer_size) // read buffer from file
    {
        rw = read_from_block(ic->i[inode_num].pointers[block_num], rw, buffer, buffer_size); // read from block

        if (rw % BLOCKSIZE_ == 0) // if block is full, get new block
        {
            block_num++; // increment block number

            if (block_num >= NUM_DIR_DATABLOCKS_) // if block number is out of range we have to use an index block
            {
                return -1;
            }
            rw_in_block = 0; // reset rw in block
        }
        bytes_read = rw - start; // increment buffer
    }

    return bytes_read;
}

/* ----------- */
/* sfs_fseek() */
/* ----------- */
/* This function seeks to a position in a file */

int sfs_fseek(int fd, int loc)
{
    printf("Seeking to %d\n", loc);
    if (ft->f[fd].active == 0)
    {
        printf("File descriptor not active\n");
        return -1;
    }
    ft->f[fd].rw = loc; // update rw in file descriptor
    return 0;
}

/* ------------ */
/* sfs_remove() */
/* ------------ */
/* This function removes a file */
int sfs_remove(char *file)
{
    return d_removeEntry(file); // get inode number of file
}

/* ------ */
/* main() */
/* ------ */
int main()
{
    mksfs(1);
    int f = sfs_fopen("some_name.txt");
    char my_data[] = "The quick brown fox jumps over the lazy dog";
    char out_data[1024];
    sfs_fwrite(f, my_data, sizeof(my_data) + 1);
    sfs_fseek(f, 0);
    sfs_fread(f, out_data, sizeof(out_data) + 1);
    printf("%s\n", out_data);
    // sfs_fclose(f);
    // int f1 = sfs_fopen("some_name.txt");
    // // int f2 = sfs_fopen("some_name2.txt");
    // char *buffer1 = "Hello World!";
    // char *buffer2 = "Goodbye World!";
    // sfs_fwrite(f1, buffer1, strlen(buffer1));
    // sfs_fwrite(f1, buffer2, strlen(buffer2));
    // sfs_fclose(f1);

    // // // print it from the disk
    // int i;
    // char *buffer4 = (void *)malloc(BLOCKSIZE_);
    // read_blocks(FIRST_DATABLOCK_ + 1, 1, buffer4);
    // for (i = 0; i < 1024; i++)
    // {
    //     printf("%c", buffer4[i]);
    // }

    // // FOR LOOP TO SEE WHAT HAPPENS WHEN WE GO BEYOND A BLOCK
    // int f = sfs_fopen("some_name.txt");
    // char *buffer6 = "notre pere qui etes aux cieu, que votre nom soit sanctifie, que votre regne vienn, que votre volonte soit faite sur la terre comme au ciel, donnez-nous aujourd'hui notre pain quotidien, pardonnez-nous nos offenses comme nous pardonnons aussi a ceux qui nous ont offensés, et ne nous soumettez pas a la tentation, mais delivrez-nous du mal, amen.";
    // sfs_fwrite(f, buffer6, strlen(buffer6));
    // sfs_fwrite(f, buffer6, strlen(buffer6));
    // sfs_fwrite(f, buffer6, strlen(buffer6));
    // sfs_fwrite(f, buffer6, strlen(buffer6));
    // sfs_fwrite(f, buffer6, strlen(buffer6));
    // sfs_fwrite(f, buffer6, strlen(buffer6));
    // sfs_fwrite(f, buffer6, strlen(buffer6));

    // sfs_fwrite(f, buffer6, strlen(buffer6));

    // sfs_fwrite(f, buffer6, strlen(buffer6));

    // sfs_fwrite(f, buffer6, strlen(buffer6));
    // sfs_fclose(f);

    // // print it from the disk
    // char *buffer5 = (void *)malloc(BLOCKSIZE_);
    // read_blocks(ic->i[0].pointers[0] + FIRST_DATABLOCK_, 1, buffer5);
    // for (i = 0; i < 1024; i++)
    // {
    //     printf("%c", buffer5[i]);
    // }
    // printf("\n ____________ \n");

    // // char *buffer5 = (void *)malloc(BLOCKSIZE_);
    // read_blocks(ic->i[0].pointers[1] + FIRST_DATABLOCK_, 1, buffer5);
    // for (i = 0; i < 1024; i++)
    // {
    //     printf("%c", buffer5[i]);
    // }
    // printf("\n ____________ \n");
    // char *buffer7 = (void *)malloc(BLOCKSIZE_);
    // read_blocks(ic->i[0].pointers[2] + FIRST_DATABLOCK_, 1, buffer7);
    // for (i = 0; i < 1024; i++)
    // {
    //     printf("%c", buffer7[i]);
    // }
    // printf("\n |||| \n");
    // char *buffer11 = (void *)malloc(BLOCKSIZE_);
    // read_blocks(ic->i[0].pointers[3] + FIRST_DATABLOCK_, 1, buffer11);
    // for (i = 0; i < 1024; i++)
    // {
    //     printf("%c", buffer11[i]);
    // }

    // f = sfs_fopen("some_name.txt");

    // // read and then print to test read
    // ft->f[f].rw = 0;
    // char *buffer10 = (void *)calloc(1, 4500);

    // sfs_fread(f, buffer10, 4500);
    // printf("\n |||||| \n");
    // for (i = 0; i < 1024; i++)
    // {
    //     printf("%c", buffer10[i]);
    // }
    // printf("\n |||||| \n");
    // for (i = 1024; i < 2048; i++)
    // {
    //     printf("%c", buffer10[i]);
    // }
    // printf("\n |||||| \n");
    // for (i = 2048; i < 3072; i++)
    // {
    //     printf("%c", buffer10[i]);
    // }
    // printf("\n |||||| \n");
    // for (i = 3072; i < 4096; i++)
    // {
    //     printf("%c", buffer10[i]);
    // }
    // printf("\n |||||| \n");

    return 0;
}
