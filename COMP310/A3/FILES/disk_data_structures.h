// McGill Fall 2023 - COMP 310 - Operating Systems
// Elliot Forcier-Poirier
// 260989602
#ifndef DISK_DATA_STRUCTURES_H
#define DISK_DATA_STRUCTURES_H

#include "constants.h"
#include <stdlib.h>
#include <math.h>

/* Bitmap */

typedef unsigned char *bitmap_p;

typedef struct
{
	bitmap_p mp;	 // Pointer to the array of bits representing the bitmap
	int numbits;	 // Total number of bits in the bitmap, number of blocks in the filesystem
	int lastfreebit; // Index of the last free bit found in the bitmap
} Bitmap;

void b_set(Bitmap *b, int i);	// Set the bit at index 'i' to 1
void b_clear(Bitmap *b, int i); // Set the bit at index 'i' to 0
int b_getbit(Bitmap *b, int i); // Get the value of the bit at index 'i'
Bitmap *b_init(int n);			// Initialize a bitmap of size 'n'
int b_getfreebit(Bitmap *b);	// Get the index of the first free bit

/* SuperBlock */

typedef struct
{
	int magic;		  // Magic number for filesystem identification
	int blocksize;	  // Size of each block in the filesystem
	int fsSize;		  // Total size of the filesystem
	int itablelength; // Length of the inode table
	int rtdir;		  // Root directory's index
} superblock;

superblock *s_init(); // Initialize the superblock

/* Inode */

struct i_node
{
	int active;						   // Indicates if the inode is active
	int size;						   // Size of the file represented by the inode
	int pointers[NUM_DIR_DATABLOCKS_]; // Direct pointers to data blocks
	int indexPointer;				   // Pointer to an indirect block
} typedef inode;

typedef struct
{
	inode i[101];  // Array of inodes
	Bitmap *iFree; // Bitmap tracking free inodes
} icache;

icache *i_initCache();			  // Initialize inode cache
int i_newEntry();				  // Create a new inode entry
icache *i_setIcache(icache *tmp); // Set the inode cache
icache *i_getIcache();			  // Get the inode cache
/*
int i_deleteEntry();						 // Delete an
int i_getSize(int index);					 // Get the size of a file from its inode
void i_setSize(int index, int sz);			 // Set the size of a file in its inode
void i_incdecSize(int index, int incdec);	 // Increment or decrement file size
int i_getPointer(int index, int pointerNum); // Get a pointer from an inode
int i_getIndPointer(int index);				 // Get indirect pointer from an inode
int i_isActive(int index);					 // Check if an inode is active
*/

/* Free Data Block */

// Structure representing free data block management
typedef struct
{
	Bitmap *dbFree; // Bitmap tracking free data blocks
} FDB;

// Function declarations for free data block operations
FDB *FDB_init();		  // Initialize free data block structure
FDB *FDB_get();			  // Get the free data block structure
void FDB_set(FDB *tmp);	  // Set the free data block structure
void FDB_setbit(int i);	  // Set a bit in the free data block bitmap
void FDB_unsetbit(int i); // Unset a bit in the free data block bitmap
int FDB_getbit(int i);	  // Get a bit from the free data block bitmap
int FDB_getfreeblock();	  // Get the index of a free data block

/* Directory Table */

struct dir_entry
{
	int active;										 // Indicates whether the directory entry is active (in use) or not
	int inode;										 // Inode number associated with the directory entry
	char *fname[MAX_FILE_NAME_ + MAX_FILE_EXT_ + 2]; // Filename (including file extension and null terminator)
} typedef dent;

typedef struct
{
	dent list[MAX_FILES_]; // Array of directory entries
	int numEntry;		   // Number of active entries in the directory
} directory;

void d_setDir(directory *tmp);		 // Set the directory structure
directory *d_getDir();				 // Retrieve the directory structure
directory *d_initDir();				 // Initialize the directory structure
int d_addEntry(int id, char *name);	 // Add a new entry to the directory
void resetDirIter();				 // Reset the directory iterator
int d_getNextDirName(char *namebuf); // Get the name of the next directory entry
int d_name2Index(char *namebuf);	 // Convert a directory name to its index
int d_getActive(int index);			 // Check if a directory entry is active
int d_getInode(int index);			 // Get the inode number of a directory entry
char *d_getName(int index);			 // Get the name of a directory entry
int d_removeEntry(int index);		 // Remove a directory entry

#endif
