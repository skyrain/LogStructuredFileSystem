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
//#include "dir.h"      
#include "log.h"      
#include "LFS.h"
#include "flash.h"
#include "File.h"

//**************************??注意 只用部分全局变量*******************

//----------------gloabal value-----------------------
extern Super_seg * super_seg;
extern Disk_cache * disk_cache;

//-------points to the logAddress that could start to write data-------
extern LogAddress * tail_log_addr;

extern Seg * seg_in_memory;


extern u_int wearlimit;

//----flash memory name---------------
extern char * fl_file;

//--------default: 1024-------------
//-------- chosen by user-------------
extern u_int sec_num;

//-------default: 2-----------------
//-------2 sectors = 1 block----------
extern u_int bk_size;

//------default: 32--------------------------
//---------chose by user-------------------
extern u_int bks_per_seg;

extern u_int seg_size;

//??注意应写程序保证若用户输入导致计算出的seg_num非整数则让用户重新输
//入
//-----------total seg num-------------
extern u_int seg_num;

//-----------一个bk 可以存多少bytes的数据-----------
//----------- in bytes-----------------------------
extern u_int bk_content_size;

//--------default :4 ------------------
//------chosen by user----------------
extern u_int cache_seg_num;

//**************************************************************


//-------------------------------------------------------------
extern Inode * inode_ifile;


extern int Dir_mkdir();
extern void Dir_Layer_Destroy();
extern int Dir_Layer_Init();
extern int Dir_Open_File();
extern int Dir_GetAttr();
extern int Dir_Create_File();
extern int Dir_Read_File();
extern int Dir_Write_File();
extern int Dir_Truncate_File();
extern int Dir_Delete_File();
extern int get_current_dir_name();
extern int Dir_Read_Dir();

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

int LFS_GetAttr(const char *path, struct stat *stbuf)
{
    printf("LFS is getting attributes \n");
    int status = Dir_GetAttr(path, stbuf);
    return status;
}

int LFS_ReadDir(const char *path, void *buf, fuse_fill_dir_t fill, off_t offset, struct fuse_file_info *fi)
{
    printf("ReadDir \n");
    int status;
    status = Dir_Read_Dir(path, buf, fill, offset, fi);
    if(status) {printf("read dir fail\n"); return status;}
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

int LFS_Truncate(const char *path, off_t offset)
{
    printf("LFS is truncating. \n");
    return Dir_Truncate_File(path, offset);
}

int LFS_Unlink(const char *path)
{
    printf("delete a file \n");
    return Dir_Delete_File(path);
}

static struct fuse_operations LFS_oper = {
    .init = LFS_Init,
    .getattr = LFS_GetAttr,
    .readdir = LFS_ReadDir,
    .create = LFS_Create,
    .open = LFS_Open,
    .opendir = LFS_OpenDir,
    .read = LFS_Read,
    .write = LFS_Write,
    .mkdir = LFS_Mkdir,
    .truncate = LFS_Truncate,
    .unlink = LFS_Unlink,
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

    fl_file = (char *)calloc(1, 8);
    strcpy(fl_file, filename);

    /*    
    //print all the arguments
    for(i = 0; i < argc; i++)
    {
    printf("%s\n", argv[i]);
    }
     */
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

    seg_num = sec_num / seg_size;
    get_slog_to_memory();

    tail_log_addr = (LogAddress *)calloc(1, sizeof(LogAddress));

    tail_log_addr->seg_no = 1;
    tail_log_addr->bk_no =1;

    sec_num = super_seg->sec_num;
    bk_size= super_seg->bk_size;
    bks_per_seg = seg_size / bk_size;

    bk_content_size = bk_size * FLASH_SECTOR_SIZE;
    BLOCK_SIZE = bk_content_size;

    inode_ifile = super_seg->checkpoint->ifile;

    get_log_to_memory(tail_log_addr);
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















