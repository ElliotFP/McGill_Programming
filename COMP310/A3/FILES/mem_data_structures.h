// McGill Fall 2023 - COMP 310 - Operating Systems
// Elliot Forcier-Poirier
// 260989602

#include "constants.h"
#include "disk_data_structures.h"

/* File Descriptor Table */

struct file_descriptor_table_entry
{
    int rw;
    // active;
    // inode = index
} typedef FTDentry;

struct file_descriptor_table
{
    FTDentry f[MAX_FILES_]; // inode = index
} typedef FDT;

void f_activate(int inode);
int f_getRW(int inode);
void f_setRW(int inode, int newrw);
void f_incdecRW(int inode, int incdec);
void f_deactivate(int inode);

/* Inode Table */

// Nothing for now, we'll see if we need it later

/* Directory Table */

struct dir_entry
{
    int active;
    int inode;
    char *fname[MAX_FILE_NAME_ + MAX_FILE_EXT_ + 2]; // two extra chars for period seperator and null terminator
} typedef dent;

typedef struct
{
    dent list[MAX_FILES_]; // should be NUM_INDODES_ as defined in sfs_api.c
    int numEntry;
} directory;

void d_setDir(directory *tmp);
directory *d_getDir();
void d_initDir();
int d_addEntry(int id, char *name);
void resetDirIter();
int d_getNextDirName(char *namebuf);
int d_name2Index(char *namebuf);
int d_getActive(int index);
int d_getInode(int index);
char *d_getName(int index);
int d_removeEntry(int index);

/* Disk Block Cache */