#include "disk_emu.h"

int mksfs(int fresh);
void sfs_ls(void);
void nfs_ls(int socket);
int sfs_open(char *name);
int sfs_close(int fileID);
int sfs_write(int fileID, char *buf, int length);
int sfs_read(int fileID, char *buf, int length);
int sfs_remove(char *file);



