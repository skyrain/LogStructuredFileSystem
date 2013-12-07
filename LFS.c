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

//**************************注意 只用部分全局变量*******************

//----------------gloabal value-----------------------
extern Super_seg * super_seg;
extern Checkpoint * checkpoint;
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
extern int Dir_Link();
extern int Dir_Statfs();
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

int LFS_Access(const char *path, int mask)
{
	return 0;
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

int LFS_Link(const char *SourcePath, const char *TargetPath){
        int status = Dir_Link(SourcePath, TargetPath);                                                                    
        if( status )
        {
                printf(" Error when doing Dir_link \n");
                return status;
        }
        return status;
}

int LFS_ReadLink(const char *path, char *buffer, size_t size){                                                            
        int status;      
        struct fuse_file_info fi;                                                                                         

        printf("LFS is reading symbolic link '%s'\n", path);                                                         

        status = LFS_Open(path, &fi); 
        if( status )     
        {
                printf("ERROR in readlink of LFS_Open\n");                                                         
                return status;    
        }
        status = LFS_Read(path, buffer, size, 0, &fi);                                                                    
        if (status != size)       
        {
                printf("ERROR in readlink of LFS_Read\n");                                                         
                return status;    
        }

        return 0;        
}

int LFS_SymLink(const char *SourcePath, const char *TargetPath){                                                          
        int status;      
        struct fuse_file_info fi; 
        mode_t mode = S_IFLNK | 0777;                                                                 

        printf("LFS is creating a symbloic link from '%s' to '%s'.\n",
                                SourcePath, TargetPath);                                                                  

        status = LFS_Create(TargetPath, mode, &fi);                                                                       
        if( status )     
        {
                printf("ERROR in LFS_SymLink in LFS_Create \n");                                                         
                return status;    
        }
        status = LFS_Write(TargetPath, SourcePath, strlen(SourcePath) + 1, 0, &fi);                                       
        if (status != strlen(SourcePath) + 1){
                printf("ERROR in SymLink of LFS_Write");   
                return -1;   
        }
        return 0;
}

int LFS_Rmdir(const char *path){
        // Remove a directory
        printf("LFS is removing directory '%s'.\n", path);
        return Dir_Delete_File(path);
}

int LFS_Rename(const char *frompath, const char *topath){                                                                 
        int status;      
        printf("LFS is renaming '%s' to '%s'\n", frompath, topath);                                                  

        status = LFS_Link(frompath, topath); 
        if( status )     
        {                
                printf("ERROR in LFS_Rename when LFS_Link \n");                                                         
                return status;    
        }
        status = LFS_Unlink(frompath);
        if( status )     
        {
                printf("ERROR in LFS_Rename when LFS_Unlink \n");                                                      
                return status;    
        }
        return status;   
}

void LFS_Destroy()
{
	printf("LFS is destroying \n");
	Dir_Layer_Destroy();
}

int LFS_Statfs(const char *path, struct statvfs *Statvfs)
{
	printf("LFS is getting FS statistics from '%s' \n", path);
	return Dir_Statfs(path, Statvfs);
}

static struct fuse_operations LFS_oper = {
    .init = LFS_Init,
    .getattr = LFS_GetAttr,
    .access = LFS_Access,
    .readdir = LFS_ReadDir,
    .create = LFS_Create,
    .open = LFS_Open,
    .opendir = LFS_OpenDir,
    .read = LFS_Read,
    .write = LFS_Write,
    .mkdir = LFS_Mkdir,
    .truncate = LFS_Truncate,
    .unlink = LFS_Unlink,
    .link = LFS_Link,
    .readlink = LFS_ReadLink,
    .symlink = LFS_SymLink,
    .rmdir = LFS_Rmdir,
    .rename = LFS_Rename,
    .destroy = LFS_Destroy,
    .statfs = LFS_Statfs,
};


int main(int argc, char *argv[])
{
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
   
    char* buffer = get_current_dir_name();
    char * s = (char *)calloc(1, 8);
    strcpy(s, filename);
    fl_file = (char *)calloc(1, strlen(buffer) + strlen(s) + 1);
    strcpy(fl_file, buffer);
    strcpy(fl_file + strlen(buffer), "/");
    strcpy(fl_file + strlen(buffer) + 1, s);
    /*    
    //print all the arguments
    for(i = 0; i < argc; i++)
    {
    printf("%s\n", argv[i]);
    }
     */
    /*
    //---------1.filename:用于打开disk-------------------
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
    */
    //--- super seg always stored at 0th seg ----
    //--- the entire 0th seg is used for super seg ---
    //--- note: since FLASH_SECTOR_SIZE 512 & sizeof(Super_seg) = 36--
    //--- just need to read one sector to get to know sec_num, seg_size--
    //-- which are of Super_seg ---------------------  
   
    void * start_buffer = calloc(1, FLASH_SECTOR_SIZE);
    Flash_Flags flags = FLASH_SILENT;
    //-- note: !!可以只open flash的一个earse block 大小 ---
    //-- 且只读一个sector的数据足矣-----------
    u_int tmp = 1;
    u_int * blocks = &tmp;
    Flash   flash_0 = Flash_Open(fl_file, flags, blocks);
    Flash_Read(flash_0, 0, 1, start_buffer);
    Flash_Close(flash_0);
    Super_seg * s_seg  = (Super_seg *)start_buffer;
    sec_num = s_seg->sec_num;
    seg_size = s_seg->seg_size;
    bk_size= s_seg->bk_size;
    bks_per_seg = seg_size / bk_size;
    seg_num = sec_num / seg_size;
    free(s_seg);

    bk_content_size = bk_size * FLASH_SECTOR_SIZE;
    BLOCK_SIZE = bk_content_size;
   
    get_slog_to_memory();
    get_checkpoint_to_memory();

    tail_log_addr = (LogAddress *)calloc(1, sizeof(LogAddress));
    tail_log_addr->seg_no = checkpoint->last_log_addr->seg_no;
    tail_log_addr->bk_no = checkpoint->last_log_addr->bk_no;

    inode_ifile = checkpoint->ifile;

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















