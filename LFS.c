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


int main(int argc, char *argv[])
{
   
    int i;
    int status = 0;
    char **nargv = NULL;
    
    //----------default 1 cache segment----------------
    cache_seg_num = 1;
    
    int ch;
    while ((ch = getopt(argc, argv, "s:")) != -1)
    {
        switch (ch) {
            case 's':
                cache_seg_num = (u_int)atoi(optarg);
                break;
           case '?':
                break;
        }
    }

    //---------------for global variables-------------------------
    //-----------------------------------------------------------
    //------from second last argv get the flashmemory name------ 
    filename = (char *)calloc(1, 8);
    strcpy(filename, argv[argc - 2]);

    //print all the arguments
    for(i = 0; i < argc; i++)
    {
        printf("%s\n", argv[i]);
    }

    //---------1. -------filename:用于打开disk-------------------

    //----- ??(phase 2) get tail_log_addr--------------
    //----- get global vairables in log.h---------------
    char* buffer = get_current_dir_name();
    char * s = "/config.ini";
    char * config = (char *)calloc(1, strlen(buffer) + strlen(s));
    strcpy(config, buffer);
    strcpy(config + strlen(buffer), s);

    FILE *fp;
    if((fp=fopen(config,"rb")) == NULL)
    {
        printf("\nopen file error");
        exit(1);
    }

    char store_seg_size[5];
    fgets(store_seg_size, 5, fp);
    seg_size = (u_int)atoi(store_seg_size);
   
    char store_sec_num[5];
    fgets(store_sec_num, 5, fp);
 
    //------------------if use fclose() 为毛有错 ????------------
    //---------暂时先不用----------------------------------------
    //  fclose(fp);
    sec_num = (u_int)atoi(store_sec_num);

    free(config);

    //--- 2.  read super seg------------------------------- 
    Flash_Flags flags = FLASH_SILENT;

    //blocks : # of blocks in the flash
    u_int tmp = sec_num / FLASH_SECTORS_PER_BLOCK;
    u_int * blocks = &tmp;
    Flash   flash = Flash_Open(filename, flags, blocks);

    //------read super seg into memory-------------------
    void * super_seg_buffer = calloc(1, seg_size * FLASH_SECTOR_SIZE);
    Flash_Read(flash, 0, seg_size, super_seg_buffer); 

   
    //----------for global variables in log.h-----------------------
    Super_seg * super_seg = (Super_seg *)super_seg_buffer;
    
    tail_log_addr = (LogAddress *)calloc(1, sizeof(LogAddress));

    tail_log_addr->seg_no = 1;
    tail_log_addr->bk_no =1;

    void * sin_buffer = calloc(1, seg_size * FLASH_SECTOR_SIZE);
    Flash_Read(flash, seg_size, seg_size, sin_buffer); 


    seg_in_memory = (Seg *)sin_buffer;
    Flash_Close(flash);

    sec_num = super_seg->sec_num;
    bk_size= super_seg->bk_size;
    bks_per_seg = seg_size / bk_size;
    seg_num = super_seg->seg_num;
    bk_content_size = bk_size * FLASH_SECTOR_SIZE;

    //-------------------------------------------------------------
   
    
    //------------- create cache once the whole system ----------
    //---------- starts to run------------------------------------
    create_cache();

#define NARGS 3
        int nargc = 2 + NARGS;
        nargv = (char **)malloc((nargc)*sizeof(char*));

        nargv[0] = argv[0];

        nargv[1] = "-f";
        nargv[2] = "-s";
        nargv[3] = "-d";
        nargv[4] = argv[ argc - 1];
        
        status = fuse_main(nargc, nargv, &LFS_oper, NULL);
        if(status){printf("fuse_main error\n"); return status;}

    return 0;
}















