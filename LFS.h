#ifndef _LFS_H
#define _LFS_H

#ifndef FUSE_USE_VERSION 
#define FUSE_USE_VERSION 26
#endif

#include <sys/stat.h>
#include "dir.h"

void *LFS_Init(struct fuse_conn_info *conn);

int LFS_GetAttr(const char *path, struct stat *stbuf);
int LFS_Access(const char *path, int mask);
int LFS_ReadDir(const char *path, void *buf, fuse_fill_dir_t fill, off_t offset, struct fuse_file_info *fi);
int LFS_Create(const char *path, mode_t mode, struct fuse_file_info *fi);

int LFS_Open(const char *path, struct fuse_file_info *fi);
int LFS_OpenDir(const char *path, struct fuse_file_info *fi);
int LFS_Read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int LFS_Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int LFS_Mkdir(const char *dir_name, mode_t mode);
int LFS_Truncate(const char *path, off_t offset);
int LFS_Unlink(const char *path);
int LFS_Link(const char *SourcePath, const char *TargetPath);
int LFS_ReadLink(const char *path, char *buffer, size_t size);
int LFS_SymLink(const char *SourcePath, const char *TargetPath);
int LFS_rmdir(const char *path);
int LFS_Rename(const char *frompath, const char *topath);
void LFS_Destroy();
int LFS_statfs(const char *path, struct statvfs *Statvfs);

#endif
