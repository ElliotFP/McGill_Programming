// McGill Fall 2023 - COMP 310 - Operating Systems
// Elliot Forcier-Poirier
// 260989602
#ifndef DISK_DATA_STRUCTURES_H
#define DISK_DATA_STRUCTURES_H

#include "constants.h"
#include <stdlib.h>
#include <math.h>

/* SuperBlock */

typedef struct __attribute__((__packed__))
{
	int magic;		  // Magic number for filesystem identification
	int blocksize;	  // Size of each block in the filesystem
	int fsSize;		  // Total size of the filesystem
	int itablelength; // Length of the inode table
	int rtdir;		  // Root directory's index
} superblock;

superblock *s_init(); // Initialize the superblock

/* Bitmap for free data blocks*/

typedef struct __attribute__((__packed__))
{
	int numbits;				  // Total number of bits in the bitmap, number of blocks in the filesystem
	int bits[DATA_BLOCKS_AVAIL_]; // Array of bits representing the bitmap
	int lastfreebit;			  // Index of the last free bit found in the bitmap
} Bitmap;

void b_set(int i);	   // Set the bit at index 'i' to 1
void b_clear(int i);   // Set the bit at index 'i' to 0
int b_getbit(int i);   // Get the value of the bit at index 'i'
Bitmap *b_init(int n); // Initialize a bitmap of size 'n'
int b_getfreebit();	   // Get the index of the first free bit

/* Inode */

struct __attribute__((__packed__)) i_node
{
	int active;						   // Indicates if the inode is active
	int size;						   // Size of the file represented by the inode
	int pointers[NUM_DIR_DATABLOCKS_]; // Direct pointers to data blocks
	int indexPointer;				   // Pointer to an indirect block
} typedef inode;

typedef struct __attribute__((__packed__))
{
	inode i[NUM_INODES_]; // Array of inodes
} icache;

icache *i_initCache();			  // Initialize inode cache
inode *init_inode(int inode_num); // Initialize an inode
int get_free_inode();			  // Get the index of the first free inode

/* Directory Table */

struct __attribute__((__packed__)) dir_entry
{
	int active;										// Indicates whether the directory entry is active (in use) or not
	int inode;										// Inode number associated with the directory entry
	char fname[MAX_FILE_NAME_ + MAX_FILE_EXT_ + 2]; // Filename (including file extension and null terminator)
} typedef dir_entry;

typedef struct __attribute__((__packed__))
{
	int numEntry;					 // Number of active entries in the directory
	int iter;						 // Iterator for directory entries
	dir_entry list[NUM_INODES_ - 1]; // Array of directory entries
} directory;

directory *d_init(int n);					 // Initialize the directory structure
int d_getFile(const char *name);			 // Get the inode number of a file in the directory
int d_addEntry(const char *name, int inode); // Add a new entry to the directory
int d_removeEntry(const char *name);		 // Remove an entry from the directory

/* File Descriptor Table */

struct __attribute__((__packed__)) file_descriptor_table_entry
{
	int rw;		// Read-Write pointer indicating the current read/write position in the file
	int active; // Indicates whether the file descriptor is active (in use) or not
	int inode;	// inode number associated with this file descriptor
	int fd;		// File descriptor number
} typedef FDTentry;

struct __attribute__((__packed__)) file_descriptor_table
{
	FDTentry f[NUM_INODES_ - 1]; // Array of file descriptor table entries
	int fd;
} typedef FDT;

FDT *f_init();				  // Get the file descriptor table
int f_createEntry(int inode); // Create a new file descriptor table entry
void f_setRW(int inode, int newrw);
void f_incdecRW(int inode, int incdec); // Increment or decrement the read/write position
void f_deactivate(int inode);			// Deactivate a file descriptor

struct block
{
	char data[BLOCKSIZE_]; // Size of a block
};
#endif
