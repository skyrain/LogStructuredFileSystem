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
#include <unistd.h>
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
u_int cachesize;

void *LFS_Init(struct fuse_conn_info *conn)
{
	int *status;

	status = (int *)calloc(1, sizeof(int));

	if(*status)
	{ printf("fail to init\n"); return status;}
    
	// Init Directory
	*status = Dir_Layer_Init(filename, cachesize);
	if(*status)
    { printf("fail to init\n"); return status;}		

    return status;
}

int LFS_Create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int status = 0;
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

//------------------?? need gcc-------------------------------------

int main(int argc, char *argv[])
{
/*
    seg_num = 1000;
    bool use_opt = false;

    cache_seg_num = 4;
    int ch;
    while ((ch = getopt(argc, argv, "s:")) != -1)
    {
        switch (ch) {
            case 's':
                if(atoi(optarg) > seg_num)
                {
                    printf("Too many cache segments!\n");
                    return 0;
                }
                cache_seg_num = (u_int)atoi(optarg);
                use_opt = true;
                break;
           case '?':
                break;
        }
    }
 
   cachesize = cache_seg_num;
   
    //------------- create cache once the whole system ----------
    //---------- starts to run------------------------------------
    create_cache();
*/
    int i;
    int status = 0;
    char **nargv = NULL;

    filename = (char *)calloc(1, 8);
    strcpy(filename, "flashmemory");

    //print all the arguments
    for(i = 0; i < argc; i++)
    {
        printf("%s\n", argv[i]);
    }
/*
    if(use_opt)
    {
        nargv = (char **)malloc((argc - 1)*sizeof(char*));

        nargv[0] = argv[0];
        nargv[1] = "-f";
        
        for(i = 3; i < argc; i++)
        {
            nargv[i - 1] = argv[i];

        }
        //---------------?? add mount point--------------------------- 
        status = fuse_main(argc - 1, nargv, &LFS_oper, NULL);
        if(status){printf("fuse_main error\n"); return status;}
    }
    else
    {


        nargv = (char **)malloc((argc + 1)*sizeof(char*));

        nargv[0] = argv[0];
        nargv[1] = "-f";
        for(i = 1; i < argc; i++)
        {
            nargv[i + 1] = argv[i];

        }
*/
        /* 
        //---------------?? add mount point--------------------------- 
        status = fuse_main(argc + 1, nargv, &LFS_oper, NULL);
        if(status){printf("fuse_main error\n"); return status;}
*/
#define NARGS 3
        int nargc = argc + NARGS;
        nargv = (char **)malloc((nargc)*sizeof(char*));

        nargv[0] = argv[0];
        nargv[1] = "-f";
        nargv[2] = "-s";
        nargv[3] = "-d";

        for(i = 1; i < argc; i++)
        {
            nargv[i + NARGS] = argv[i];

        }
        //---------------?? add mount point--------------------------- 
        status = fuse_main(nargc, nargv, &LFS_oper, NULL);
        if(status){printf("fuse_main error\n"); return status;}
        
//    }



    return 0;
}


















