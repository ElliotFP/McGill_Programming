// McGill Fall 2023 - COMP 310 - Operating Systems
// Elliot Forcier-Poirier
// 260989602

#include "mem_data_structures.h"
#include "constants.h"

/* File Descriptor Table */

FDT *ft; // Global pointer to File Descriptor Table

void f_init() // Initialize the file descriptor table
{
    int x;
    for (x = 0; x < MAX_FILES_; x++) // Iterate over all possible file descriptors
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

/* Inode Table */

// Placeholder for future implementation
