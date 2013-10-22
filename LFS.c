/*
 * 	LFS.C
 * 	created on 20 Oct.
 * 	This is the LFS c code
 */

#ifndef FUSE_USE_VERSION 
#define FUSE_USE_VERSION 26
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>     
#include <math.h> 
#include <errno.h>
#include <fcntl.h>
#include <string.h>   
#include "dir.h"      
#include "log.h"      

// Initialize FS, return value will pass in the fuse_context to all
// file operations.

char *filename;

void *LFS_Init(struct fuse_conn_info *conn)
{
	int *status;

	status = (int *)calloc(1, sizeof(int));

	if(*status)
	{ printf("fail to init\n"); return status;}

	// Init Directory
	*status = Dir_Layer_Init(filename);
	if(*status)
	{ printf("fail to init\n"); return status;}		
	
	return status;
}

int LFS_Create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int status;
	struct fuse_context* context = fuse_get_context();
	Dir_Create_File(path, mode, context->uid, context->gid, fi);
	LFS_Open(path, fi);
	if(status) {printf("LFS create fail\n"); return status;}
	return status;
}
	
int LFS_Open(const char *path, struct fuse_file_info *fi)
{
	return Dir_Open_File(path, fi);
}

int LFS_OpenDir(const char *path, struct fuse_file_info *fi)
{
	return Dir_Open_File(path, fi);
}

int LFS_Read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	return Dir_Read_File(path, buf, size, offset, fi);
}

int LFS_Write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	return Dir_Write_File(path, buf, size, offset, fi);
}

int LFS_Mkdir(const char *dir_name, mode_t mode)
{
	struct fuse_context *context = fuse_get_context();
	return Dir_mkdir(dir_name, mode, context->uid, context->gid);
}

static struct fuse_operations LFS_oper = {
        .init = LFS_Init,
        .create = LFS_Create,
        .open = LFS_Open,
	.opendir = LFS_OpenDir,
	.read = LFS_Read,
	.write = LFS_Write,
        .mkdir = LFS_Mkdir,
};

int main(int argc, char *argv[])
{
        int i;
        int status = 0;
        char **nargv = NULL;
	
	//print all the arguments
        for(i=0; i<argc; i++)
        {
                printf("%s\n", argv[i]);
        }

        nargv = (char **)malloc((argc+1)*sizeof(char*));

        nargv[0] = argv[0];
        nargv[1] = "-f";
        for(i=1; i<argc; i++)
                nargv[i+1] = argv[i];

        status = fuse_main(argc+1, nargv, &LFS_oper, NULL);
	if(status){printf("fuse_main error\n"); return status;}
}




















