// McGill Fall 2023 - COMP 310 - Operating Systems
// Elliot Forcier-Poirier
// 260989602

#include "constants.h"
#include "bitmap.h"
#include "indirectblock.h"

struct i_node
{
	int active;
	int size;
	int pointers[NUM_DIR_DATABLOCKS_];
	int indPointer;
} typedef inode;

typedef struct
{
	inode i[101];
	Bitmap iFree;
} icache;

icache *i_initCache();
int i_deleteEntry();
int i_newEntry();
void i_setIcache(icache tmp);
icache *i_getIcache();
int i_getSize(int index);
void i_setSize(int index, int sz);
void i_incdecSize(int index, int incdec);
int i_getPointer(int index, int pointerNum);
int i_getIndPointer(int index);
int i_isActive(int index);
