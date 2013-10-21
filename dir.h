/*
 * dir.h
 * Created on: oct 17, 2013
 * This is the directory layer h code
 */

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26

#include <sys/types.h>
#include <sys/stat.h>
#include <fuse.h>
#include <string.h>
#include <errno.h>
#include "log.h"
#include "File.h"
//more need from tianyu
#define ROOT_INO 0
#define UNDEFINE_FILE -2

int Dir_Layer_Init(char *filename) //cache and checkPointPeriod save for Phase 2

int Dir_mkdir(const char *dirName, mode_t mode, uid_t, gid_t gid);

int Dir_Read_File(const char *path, char *buf, size_t size, off_t offset);

int Dir_Open_File(const char *path, struct fuse_file_info *fi);

int Dir_Create_File(const char *path, mode_t mode, uid_t uid, gid_t gid, struct fuse_file_info *fi);

void Dir_layer_De();

int Get_Dir_Inode(const char *path, inode **returnNode, char *filename);

int Dir_Write_file(Inode *myNode, const char *buf, size_t size, off_t offset);

int Add_File_To_Directory(const char *path, int inum);

int Flush_Ino(int inum);

int Get_Inode(const char *dir_name, Inode **dirNode);

int Get_New_Ino();

typedef struct DirEntry
{
	char filename[FILE_NAME_LENGTH];
	int inum;
}DirEntry;

DirEntry *Get_Dir(Inode *dirNode, int *numfiles);

#endif
