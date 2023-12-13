// McGill Fall 2023 - COMP 310 - Operating Systems
// Elliot Forcier-Poirier
// 260989602

#include <stdint.h>
#include <stdlib.h>
#include "disk_data_structures.h"
#include "constants.h"
#include "disk_emu.h"

/* Bitmap */

Bitmap *b; // Global pointer to Bitmap

Bitmap *getBitmap() { return b; }

void b_set(Bitmap *b, int i) { b->bits[i] = 1; }

void b_clear(Bitmap *b, int i) { b->bits[i] = 0; }

int b_getbit(Bitmap *b, int i) { return b->bits[i]; }

Bitmap *b_init(int n) // Initialize a bitmap of size n
{
    // Allocate memory for the Bitmap structure and the bitmap itself.
    Bitmap *b = (Bitmap *)calloc(1, sizeof(Bitmap));

    // initialize bitmap to all 1s (all bits are free)
    int x;
    for (x = 0; x < n; x++)
    {
        b->bits[x] = 1;
    }
    printf("Bitmap initialized with %d bytes\n", n);

    // Initialize the bitmap fields.
    b->numbits = n;
    b->lastfreebit = 0;
    return b; // Return the bitmap pointer
}

int b_getfreebit(Bitmap *b) // Get the index of the first free bit
{
    int x;
    for (x = 0; x < b->numbits; x++) // iterate over all bits
    {
        if (b->bits[x] == 1) // if a free bit is found, return its index
        {
            b->lastfreebit = x;
            return x;
        }
    }
    return -1;
}

/* SuperBlock */

superblock *s_init() // Initialize the superblock
{
    printf("Initializing superblock\n");
    superblock *s = (superblock *)malloc(sizeof(superblock));
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
    printf("Superblock initialized\n");

    return s; // Return the superblock pointer
}

/* Inode */

icache *ic; // declare inode cache pointer

icache *get_icache() { return ic; }

icache *i_initCache() // initialize inode cache
{
    printf("Initializing inode cache\n");
    ic = (icache *)calloc(1, sizeof(icache)); // allocate memory for inode cache
    int x;                                    // iterator

    for (x = 0; x < NUM_INODES_ - 1; x++) // iterate over all inodes
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
    // setup directory inode
    ic->i[0].active = 1;
    ic->i[0].size = 0;

    // point to the first data block of the directory
    write_blocks(FIRST_DATABLOCK_, 1, d_initDir(NUM_INODES_ - 1));

    printf("Inode cache initialized\n");
    return ic; // return inode cache pointer
}

/* Directory Table */

directory *d;        // Global pointer to Directory Table
int dirIterIndx = 0; // Index for iterating through the directory

directory *get_dir() { return d; }

directory *d_initDir(int maxEntries) // Initialize the directory structure
{
    printf("Initializing directory\n");

    int x;
    d = (directory *)calloc(1, sizeof(directory)); // Allocate memory for the directory

    d->numEntry = maxEntries; // Reset the number of entries in the directory

    for (x = 0; x < NUM_INODES_ - 1; x++) // Initialize each directory entry
    {
        d->list[x].active = 0; // Mark each directory entry as inactive
    }
    d->iter = 0; // Reset directory iteration index
    return d;
}

/* File Descriptor Table */

FDT *ft; // Global pointer to File Descriptor Table

void f_init() // Initialize the file descriptor table
{
    int x;
    for (x = 0; x < NUM_INODES_ - 1; x++) // Iterate over all possible file descriptors
    {
        ft->f[x].rw = 0;     // Initialize Read/Write pointer to 0
        ft->f[x].active = 0; // Mark the file descriptor as inactive
    }
}

void f_activate(int inode) // Activate a file descriptor
{
    ft->f[inode].active = 1;
}

int f_getRW(int inode) // Get the current read/write position of a file descriptor
{
    return ft->f[inode].rw;
}

int f_isActive(int inode) // Check if a file descriptor is active
{
    return ft->f[inode].active;
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
}
