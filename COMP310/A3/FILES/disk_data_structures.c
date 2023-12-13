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
    Bitmap *b = (Bitmap *)calloc(1, sizeof(Bitmap));
    b->mp = (bitmap_p)calloc(1, bytes);

    // initialize bitmap to all 1s (all bits are free)
    int x;
    for (x = 0; x < n; x++)
    {
        b_set(b, x);
    }
    printf("Bitmap initialized with %d bits\n", n);

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

icache *i_initCache() // initialize inode cache
{
    printf("Initializing inode cache\n");
    ic = (icache *)malloc(sizeof(icache)); // allocate memory for inode cache
    int x;                                 // iterator

    for (x = 0; x < MAX_FILES_; x++) // iterate over all inodes
    {
        ic->i[x].active = 0;
        ic->i[x].size = 0;
        // set all pointers to -1
        for (int y = 0; y < NUM_DIR_DATABLOCKS_; y++)
        {
            ic->i[x].pointers[y] = -1;
        }
        ic->i[x].indexPointer = -1;
    }
    ic->iFree = b_init(NUM_INODES_); // initialize bitmap

    // setup directory inode
    b_set(ic->iFree, 0);
    ic->i[0].active = 1;
    ic->i[0].size = 0;

    printf("Inode cache initialized\n");
    return ic; // return inode cache pointer
}

icache *i_getIcache() // get inode cache pointer
{
    return ic;
}

icache *i_setIcache(icache *tmp) // set inode cache pointer
{
    ic = tmp;
    return ic;
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
void FDB_clearbit(int i) // set bit at index i to 0
{
    b_clear(fd->dbFree, i);
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
    printf("Initializing free data block\n");
    FDB *fd = (FDB *)malloc(sizeof(FDB));
    if (fd == NULL)
    {
        printf("Error allocating memory for free data block\n");
        return NULL;
    }

    fd->dbFree = b_init(FBM_BLOCK_); // initialize bitmap
    // block off super block
    if (fd->dbFree == NULL)
    {
        printf("Error allocating memory for free data block bitmap\n");
        return NULL;
    }

    // b_set(&fd->dbFree, SB_BLOCK_);
    int x;
    // block off inode table
    for (x = ICACHE_BLOCK_START_; x < ICACHE_BLOCK_END_ + 1; x++)
    {
        b_set(fd->dbFree, x);
    }

    // block off free data block map
    // b_set(&fd->dbFree, FDB_BLOCK_);
    printf("Free data block initialized\n");

    return fd; // return free data block pointer
}

/* Directory Table */

directory *d;        // Global pointer to Directory Table
int dirIterIndx = 0; // Index for iterating through the directory

void d_setDir(directory *tmp) // Set the directory structure
{
    d = tmp;
}

directory *d_getDir() // Retrieve the directory structure
{
    return d;
}

directory *d_initDir() // Initialize the directory structure
{
    printf("Initializing directory\n");
    int x;
    d = (directory *)malloc(sizeof(directory)); // Allocate memory for the directory

    d->numEntry = 0; // Reset the number of entries in the directory

    for (x = 0; x < MAX_FILES_; x++) // Initialize each directory entry
    {
        d->list[x].active = 0; // Mark each directory entry as inactive
    }
    dirIterIndx = 0; // Reset directory iteration index
    return d;
}

int d_addEntry(int id, char *name) // Add a new entry to the directory
{
    if (d->numEntry >= MAX_FILES_) // Check if directory is full
    {
        return -1; // Return error if directory is full
    }
    // Add new entry
    d->list[d->numEntry].active = 1;          // Mark entry as active
    d->list[d->numEntry].inode = id;          // Set inode number
    strcpy(d->list[d->numEntry].fname, name); // Copy file name
    d->numEntry++;                            // Increment number of entries
    return 0;                                 // Success
}

void resetDirIter() // Reset the directory iterator
{
    dirIterIndx = 0;
}

int d_getNextDirName(char *namebuf) // Get the name of the next directory entry
{
    if (dirIterIndx >= MAX_FILES_) // Check if iterator has reached the end
    {
        resetDirIter(); // Reset iterator
        return -1;      // Return error
    }
    else if (d->list[dirIterIndx].active) // Check if current entry is active
    {
        strcpy(namebuf, d->list[dirIterIndx].fname); // Copy file name
        return dirIterIndx++;                        // Return index and increment iterator
    }
    return -1; // Return error if current entry is not active
}

int d_name2Index(char *namebuf) // Convert directory name to its index
{
    char *tmp;
    int inx = -1;
    resetDirIter();
    while ((inx = d_getNextDirName(tmp)) != -1)
    {
        if (strcmp(tmp, namebuf) == 0) // Compare names
        {
            return inx; // Return index if names match
        }
    }
    return -1; // Return error if name not found
}

int d_getActive(int index) // Check if a directory entry is active
{
    return d->list[index].active;
}

int d_getInode(int index) // Get the inode number of a directory entry
{
    return d->list[index].inode; // Return inode number
}

char *d_getName(int index) // Get the name of a directory entry
{
    return d->list[index].fname; // Return pointer to file name
}

int d_removeEntry(int index) // Remove a directory entry
{
    if (d_getActive(index) == 0) // Check if entry is active
        return -1;

    d->numEntry--;
    int x;

    for (x = index; x < d->numEntry; x++) // iterate through all entries after the one to be removed
    {
        if (d_getActive(x + 1) == 0) // if the next entry is inactive, break
        {
            d->list[x].active = 0; // mark the current entry as inactive
            break;
        }
        else
        {
            strcpy(d->list[x].fname, d_getName(x + 1)); // copy the next entry's name to the current entry
            d->list[x].inode = d_getInode(x + 1);
        }
    }
    return 0;
}
