// McGill Fall 2023 - COMP 310 - Operating Systems
// Elliot Forcier-Poirier
// 260989602

#include <stdint.h>
#include <stdlib.h>
#include "structures.h"
#include "constants.h"
#include "disk_emu.h"

icache *ic;
superblock *s;
Bitmap *b;
directory *d;
FDT *ft;

/* Bitmap */

void b_set(int i) { b->bits[i] = 1; }

void b_clear(int i) { b->bits[i] = 0; }

int b_getbit(int i) { return b->bits[i]; }

Bitmap *b_init(int n) // Initialize a bitmap of size n
{
    printf("Initializing bitmap\n");
    b = (Bitmap *)calloc(1, sizeof(Bitmap)); // Allocate memory for the bitmap
    // initialize bitmap to all 1s (all bits are free)
    int x;
    for (x = 0; x < n; x++)
    {
        b->bits[x] = 1;
    }

    // Initialize the bitmap fields.
    b->numbits = n;
    b->lastfreebit = 0;
    return b; // Return the bitmap pointer
}

int b_getfreebit() // Get the index of the first free
{
    int x;
    for (x = 0; x < b->numbits; x++) // iterate over all bits
    {
        if (b->bits[x] == 1) // if a free bit is found, return its index
        {
            b->bits[x] = 0;
            return x;
        }
    }
    return -1;
}

/* SuperBlock */

superblock *s_init() // Initialize the superblock
{
    s = (superblock *)calloc(1, sizeof(superblock)); // Allocate memory for the superblock
    printf("Initializing superblock\n");
    if (s == NULL)
    {
        printf("Error allocating memory for superblock\n");
        return NULL;
    }

    // Allocate fields from constants.h
    s->magic = SB_MAGIC_;
    s->blocksize = BLOCKSIZE_;
    s->fsSize = NUM_BLOCKS_;
    s->itablelength = NUM_INODES_;
    s->rtdir = 0;

    return s; // Return the superblock pointer
}

/* Inode */

icache *i_initCache() // initialize inode cache
{
    printf("Initializing inode cache\n");
    int x; // iterator

    ic = (icache *)calloc(1, sizeof(icache)); // allocate memory for inode cache

    for (x = 0; x < NUM_INODES_; x++) // iterate over all inodes
    {
        ic->i[x].active = 0;
        ic->i[x].size = 0;
        // set all pointers to -1
        // for (int y = 0; y < NUM_DIR_DATABLOCKS_; y++)
        // {
        //     ic->i[x].pointers[y] = -1;
        // }
        // ic->i[x].indexPointer = -1;
    }

    return ic; // return inode cache pointer
}

inode *init_inode(int inode_num) // initialize an inode
{
    inode *i = (inode *)calloc(1, sizeof(inode)); // allocate memory for inode
    i->active = 1;
    i->size = 0;
    return i;
}

int get_free_inode() // Get the index of the first free inode
{
    int x;
    for (x = 0; x < NUM_INODES_ - 1; x++) // iterate over all inodes
    {
        if (ic->i[x].active == 0) // if an inactive inode is found, return its index
        {
            ic->i[x].active = 1;
            return x;
        }
    }
    return -1; // if no free inode is found, return -1
}

/* Directory Table */

directory *d_init(int maxEntries) // Initialize the directory structure
{
    d = (directory *)calloc(1, sizeof(directory)); // Allocate memory for the directory
    printf("Initializing directory\n");

    d->numEntry = maxEntries; // Reset the number of entries in the directory

    int x;
    for (x = 0; x < NUM_INODES_ - 1; x++) // Initialize each directory entry
    {
        d->list[x].active = 0; // Mark each directory entry as inactive
    }
    d->iter = 0; // Reset directory iteration index

    // store the directory in the first inode
    ic->i[DIR_INODE_].active = 1;
    ic->i[DIR_INODE_].size = sizeof(d);

    int freebit = b_getfreebit();            // get a free data block (first one)
    ic->i[DIR_INODE_].pointers[0] = freebit; // first data block

    // write to disk
    write_blocks(freebit + DATA_BLOCKS_AVAIL_, 1, (void *)d);
    return d;
}

int d_getFile(const char *name) // Get the inode number of a file
{
    int x;
    for (x = 0; x < NUM_INODES_ - 1; x++) // Iterate over all directory entries
    {
        if (strcmp(d->list[x].fname, name) == 0) // If the filename matches, return the inode number
        {
            return d->list[x].inode;
        }
    }
    return -1; // If the filename is not found, return -1
}

int d_addEntry(const char *name, int inode) // Add a directory entry
{
    int x;
    for (x = 0; x < NUM_INODES_ - 1; x++) // Iterate over all directory entries
    {
        if (d->list[x].active == 0) // If an inactive directory entry is found, add the new entry
        {
            d->list[x].active = 1; // Mark the directory entry as active
            d->list[x].inode = inode;
            strcpy(d->list[x].fname, name); // Copy the filename into the directory entry

            int last_block_pointer = ic->i[DIR_INODE_].size / BLOCKSIZE_;
            int rw_in_block = ic->i[DIR_INODE_].size % BLOCKSIZE_;

            if (rw_in_block + sizeof(d) > BLOCKSIZE_) // not enough space in the last block
            {
                last_block_pointer++;
                ic->i[DIR_INODE_].pointers[last_block_pointer] = last_block_pointer;
                write_blocks(ic->i[DIR_INODE_].pointers[last_block_pointer] + FIRST_DATABLOCK_, 1, d);
                ic->i[DIR_INODE_].size = last_block_pointer * BLOCKSIZE_ + sizeof(d);
            }
            else // enough space in the last block
            {
                char *block = (char *)malloc(BLOCKSIZE_);
                read_blocks(ic->i[DIR_INODE_].pointers[last_block_pointer] + FIRST_DATABLOCK_, 1, (void *)block);

                memcpy(block + rw_in_block, d, sizeof(&d));

                write_blocks(ic->i[DIR_INODE_].pointers[last_block_pointer] + FIRST_DATABLOCK_, 1, block);
                free(block);
                ic->i[DIR_INODE_].size += sizeof(d);
            }

            return 0;
        }
    }
    return -1; // If no inactive directory entry is found, return -1
}

int d_removeEntry(const char *name) // Remove a directory entry
{
    int x;
    for (x = 0; x < NUM_INODES_ - 1; x++) // Iterate over all directory entries
    {
        if (strcmp(d->list[x].fname, name) == 0) // If the filename matches, remove the entry
        {
            d->list[x].active = 0; // Mark the directory entry as inactive

            // unallocate the data blocks
            for (int y = 0; y < NUM_DIR_DATABLOCKS_; y++)
            {

                if (ic->i[d->list[x].inode].pointers[y] != -1)
                {
                    b->bits[ic->i[d->list[x].inode].pointers[y]] = 1;
                }
            }

            // unallocate the inode
            ic->i[d->list[x].inode].active = 0;
            ic->i[d->list[x].inode].size = 0;
            for (int y = 0; y < NUM_DIR_DATABLOCKS_; y++)
            {
                ic->i[d->list[x].inode].pointers[y] = -1;
            }
            ic->i[d->list[x].inode].indexPointer = -1;

            return 0;
        }
    }
    return -1; // If the filename is not found, return -1
}

/* File Descriptor Table */

FDT *f_init() // Initialize the file descriptor table
{
    ft = (FDT *)calloc(1, sizeof(FDT)); // Allocate memory for the file descriptor table
    int x;
    for (x = 0; x < NUM_INODES_ - 1; x++) // Iterate over all possible file descriptors
    {
        ft->f[x].rw = 0;     // Initialize Read/Write pointer to 0
        ft->f[x].active = 0; // Mark the file descriptor as inactive
        ft->f[x].inode = -1; // Set the inode number to -1
        ft->f[x].fd = x;     // Set the file descriptor number
    }
    return ft; // Return the file descriptor table pointer
}

int f_createEntry(int inode) // Create a new file descriptor table entry
{
    FDTentry *f = (FDTentry *)malloc(sizeof(FDTentry)); // Allocate memory for the file descriptor table entry
    printf("Creating file descriptor table entry\n");

    for (int x = 0; x < NUM_INODES_ - 1; x++) // Iterate over all possible file descriptors
    {
        if (ft->f[x].active == 0) // If an inactive file descriptor is found, use it
        {
            ft->f[x].rw = ic->i[inode].size; // Set the read/write pointer to the end of the file
            ft->f[x].active = 1;             // Mark the file descriptor as active
            ft->f[x].inode = inode;          // Set the inode number
            return ft->f[x].fd;
        }
    }
    return -1; // If no inactive file descriptor is found, return -1
}

void f_setRW(int inode, int newrw) // Set the rw pointer for a specific inode
{
    ft->f[inode].rw = newrw;
}

void f_incdecRW(int inode, int incdec) // incdec the rw pointer for a specific inode
{
    ft->f[inode].rw += incdec; // Adjust the rw pointer
                               // TODO: Add error handling to check for out-of-bounds access
}

void f_deactivate(int inode) // Deactivate a file descriptor
{
    ft->f[inode].active = 0;
    ft->f[inode].rw = 0;
    ft->f[inode].inode = -1;
}
