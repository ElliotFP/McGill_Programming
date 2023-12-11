// McGill Fall 2023 - COMP 310 - Operating Systems
// Elliot Forcier-Poirier
// 260989602

#include <stdint.h>
#include <stdlib.h>
#include "disk_data_structures.h"

/* Bitmap */

void b_set(Bitmap *b, int i) // set the bit at index i to 1
{
    // Dividing 'i' by 8 finds the corresponding byte,
    // and 'i & 7' finds the specific bit within that byte. The bit is then set using bitwise OR.
    b->mp[i / 8] |= 1 << (i & 7);
}

void b_clear(Bitmap *b, int i) // set the bit at index i to 0
{
    // Clears the ith bit. Similar to b_set, but uses bitwise AND with the negated bit mask.
    b->mp[i / 8] &= ~(1 << (i & 7));
}

int b_getbit(Bitmap *b, int i) // Get the value of a bit in the bitmap
{
    // Uses bitwise AND to isolate the bit and then checks if the result is non-zero.
    return b->mp[i / 8] & (1 << (i & 7)) ? 1 : 0;
}

Bitmap *b_init(int n) // Initialize a bitmap of size n
{
    // Calculate the exact number of bytes needed to store n bits.
    double exactbytes = (n / 8.0);
    int bytes = (int)ceil(exactbytes); // Round up to the nearest integer.

    // Allocate memory for the Bitmap structure and the bitmap itself.
    Bitmap *b = (Bitmap *)malloc(sizeof(Bitmap));
    b->mp = (bitmap_p)malloc(bytes);

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
        // Check if the bit (lastfreebit + x) % numbits is free (i.e., 0).
        if (b_getbit(b, (b->lastfreebit + x) % b->numbits) == 0)
        {
            // Update lastfreebit and return its index.
            b->lastfreebit = (b->lastfreebit + x) % b->numbits;
            return b->lastfreebit;
        }
    }
    // If no free bit is found, return -1.
    return -1;
}

/* SuperBlock */

superblock *s_init() // Initialize the superblock
{
    superblock *s;

    // Allocate fields from constants.h
    s->magic = SB_MAGIC_;
    s->blocksize = BLOCKSIZE_;
    s->fsSize = NUM_BLOCKS_;
    s->itablelength = NUM_INODES_;
    s->rtdir = 0;

    return s; // Return the superblock pointer
}

/* Inode */

icache *ic; // declare inode cache pointer

icache *i_initCache() // initialize inode cache
{
    int x; // iterator

    for (x = 1; x < MAX_FILES_; x++) // iterate over all inodes
    {
        ic->i[x].active = 0;
        ic->i[x].size = 0;
    }

    ic->iFree = b_init(NUM_INODES_); // initialize bitmap

    // setup directory inode
    b_set(ic->iFree, 0);
    ic->i[0].active = 1;

    return ic; // return inode cache pointer
}

icache *i_getIcache() // get inode cache pointer
{
    return ic;
}

icache *i_setIcache(icache *tmp) // set inode cache pointer
{
    ic = tmp;
}

int i_newEntry() // create a new inode entry
{
    int index = b_getfreebit(ic->iFree); // get free inode index

    if (index < 0) // no more free inodes
        return -1;

    // set inode as active
    b_set(ic->iFree, index);
    ic->i[index].active = 1;

    return index; // return inode index
}

/*
void i_deleteEntry(int index) // TODO error handling
{
    b_unset(ic->iFree, index);
    ic->i[index].active = 0;
    ic->i[index].size = 0;
    // TODO set pointers to null
}

int i_getSize(int index) // TODO error handling
{
    return ic->i[index].size;
}

void i_setSize(int index, int sz) // TODO error handling
{
    ic->i[index].size = sz;
}

void i_incdecSize(int index, int incdec)
{
    i_setSize(index, i_getSize(index) + incdec);
}

int i_getPointer(int index, int pointerNum) // TODO error handling
{
    return ic->i[index].pointers[pointerNum];
}

int i_getIndPointer(int index) // TODO error handling
{
    return ic->i[index].indPointer;
}

int i_isActive(int index) // TODO error handling
{
    return ic->i[index].active;
}
*/

/* Free Data Block */

FDB *fd; // declare free data block pointer

FDB *FDB_get() // get free data block pointer
{
    return fd;
}
void FDB_set(FDB *tmp) // set free data block pointer
{
    fd = tmp;
}
void FDB_setbit(int i) // set bit at index i to 1
{
    b_set(fd->dbFree, i);
}
void FDB_unsetbit(int i) // set bit at index i to 0
{
    b_unset(fd->dbFree, i);
}
int FDB_getbit(int i) // get bit at index i
{
    return b_getbit(fd->dbFree, i);
}

int FDB_getfreeblock() // get index of first free bit
{
    return b_getfreebit(fd->dbFree);
}

FDB *FDB_init() // initialize free data block structure
{
    fd->dbFree = *b_init(NUM_BLOCKS_);

    // block off super block
    FDB_setbit(SB_BLOCK_);
    int x;

    // block off inode table
    for (x = ICACHE_BLOCK_START_; x < ICACHE_BLOCK_END_ + 1; x++)
    {
        FDB_setbit(x);
    }

    // block off free data block map
    FDB_setbit(FDB_BLOCK_);

    return fd; // return free data block pointer
}