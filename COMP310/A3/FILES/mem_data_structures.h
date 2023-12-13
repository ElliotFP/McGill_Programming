// McGill Fall 2023 - COMP 310 - Operating Systems
// Elliot Forcier-Poirier
// 260989602

#include "constants.h"
#include "disk_data_structures.h"

/* File Descriptor Table */

struct file_descriptor_table_entry
{
    int rw;     // Read-Write pointer indicating the current read/write position in the file
    int active; // Indicates whether the file descriptor is active (in use) or not
    int inode;  // inode number associated with this file descriptor
} typedef FTDentry;

struct file_descriptor_table
{
    FTDentry f[MAX_FILES_]; // Array of file descriptor table entries
} typedef FDT;

void f_activate(int inode);             // Activate a file descriptor
int f_getRW(int inode);                 // Get the current read/write position of a file descriptor
void f_setRW(int inode, int newrw);     // Set the read/write position of a file descriptor
void f_incdecRW(int inode, int incdec); // Increment or decrement the read/write position
void f_deactivate(int inode);           // Deactivate a file descriptor

/* Inode Table */

// Placeholder for Inode Table structure
// Currently empty, but consider defining an inode table structure here

/* Disk Block Cache */

// Placeholder for Disk Block Cache structure
// Consider defining a structure for caching disk blocks to improve I/O efficiency
