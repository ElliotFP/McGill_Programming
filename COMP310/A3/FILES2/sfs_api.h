#ifndef SFS_API_H
#define SFS_API_H

extern int db_start;
extern int num_db;
extern int root_inode_num;

void mksfs(int);

int sfs_getnextfilename(char *);

int sfs_getfilesize(const char *);

int sfs_fopen(char *);

int sfs_fclose(int);

int sfs_fwrite(int, const char *, int);

int sfs_fread(int, char *, int);

int sfs_fseek(int, int);

int sfs_remove(char *);

#endif
