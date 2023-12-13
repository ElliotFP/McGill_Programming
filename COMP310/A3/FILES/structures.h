// McGill Fall 2023 - COMP 310 - Operating Systems
// Elliot Forcier-Poirier
// 260989602
#ifndef DISK_DATA_STRUCTURES_H
#define DISK_DATA_STRUCTURES_H

#include "constants.h"
#include <stdlib.h>
#include <math.h>

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

/* Bitmap for free inodes*/

typedef struct
{
	int numbits;				  // Total number of bits in the bitmap, number of blocks in the filesystem
	int bits[DATA_BLOCKS_AVAIL_]; // Array of bits representing the bitmap
	int lastfreebit;			  // Index of the last free bit found in the bitmap
} Bitmap;

Bitmap *getBitmap();			// Get the free data block bitmap
void b_set(Bitmap *b, int i);	// Set the bit at index 'i' to 1
void b_clear(Bitmap *b, int i); // Set the bit at index 'i' to 0
int b_getbit(Bitmap *b, int i); // Get the value of the bit at index 'i'
Bitmap *b_init(int n);			// Initialize a bitmap of size 'n'
int b_getfreebit(Bitmap *b);	// Get the index of the first free bit
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
	inode i[NUM_INODES_]; // Array of inodes
} icache;

icache *get_icache();  // Get the inode cache
icache *i_initCache(); // Initialize inode cache

/* Directory Table */

struct dir_entry
{
	int active;										 // Indicates whether the directory entry is active (in use) or not
	int inode;										 // Inode number associated with the directory entry
	char *fname[MAX_FILE_NAME_ + MAX_FILE_EXT_ + 2]; // Filename (including file extension and null terminator)
} typedef dir_entry;

typedef struct
{

	int numEntry;					 // Number of active entries in the directory
	int iter;						 // Iterator for directory entries
	dir_entry list[NUM_INODES_ - 1]; // Array of directory entries
} directory;

directory *d_initDir(); // Initialize the directory structure
directory *get_dir();	// Get the directory structure
// int d_addEntry(int id, char *name);	 // Add a new entry to the directory
// int d_getNextDirName(char *namebuf); // Get the name of the next directory entry
// int d_name2Index(char *namebuf);	 // Convert a directory name to its index
// int d_removeEntry(int index);		 // Remove a directory entry

/* File Descriptor Table */

struct file_descriptor_table_entry
{
	int rw;		// Read-Write pointer indicating the current read/write position in the file
	int active; // Indicates whether the file descriptor is active (in use) or not
	int inode;	// inode number associated with this file descriptor
} typedef FTDentry;

struct file_descriptor_table
{
	FTDentry f[NUM_INODES_ - 1]; // Array of file descriptor table entries
} typedef FDT;

void f_activate(int inode);				// Activate a file descriptor
int f_getRW(int inode);					// Get the current read/write position of a file descriptor
void f_setRW(int inode, int newrw);		// Set the read/write position of a file descriptor
void f_incdecRW(int inode, int incdec); // Increment or decrement the read/write position
void f_deactivate(int inode);			// Deactivate a file descriptor

#endif
