int nfs_mount(char *filesys, char *host, int port);
int nfs_umount(char *filesys);
void nfs_ls(char *filesys);
int nfs_open(char *filesys, char *name);
int nfs_close(int fd);
int nfs_write(int fd, char *buf, int length);
int nfs_read(int fd, char *buf, int length);
int nfs_remove(char *filesys, char *name);
