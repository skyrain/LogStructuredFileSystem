#ifndef LOG_H
#define LOG_H


#include "flash.h"
#include <time.h>
#include <sys/types.h>

#define bool int
#define false 0
#define true 1


#define DIRECT_BK_NUM 4
#define FILE_NAME_LENGTH 8
//#define BLOCK_SIZE FLASH_BLOCK_SIZE // temp for file layer test

//#define N_BEGIN_BLOCK 2



//------------------structure definition---------------------

//----------------------------------------------------
//-------malloc(bk_size * FLASH_SECTOR_SIZE);
//------------bk_content 可以存above大小的data
typedef struct Block
{
    //-----存file data------
    //----可以存的大小为
    void * bk_content;   

}Block;

//list "bk_no"th block in certain seg
//it contaiins which file's which file block data
typedef struct Seg_sum_entry
{
    u_int bk_no;
    u_int file_no;            
    u_int file_bk_no;
    struct Seg_sum_entry * next;

}Seg_sum_entry;

//每个seg有 bks_per_seg 个 blocks
typedef struct Seg_sum_bk
{
    //该Seg_sum_bk存在seg的哪一个block里面
    u_int bk_no;        
    Seg_sum_entry * seg_sum_entry;

    //之后加入uid用于判断一个seg的某个block是否alive
    //??

}Seg_sum_bk;

//Suppose only 1 Seg_sum_bk as Begin_bk in Seg
//---------一个 Begin_bk 占一个block size 大小
//------malloc(bk_size * FLASH_SECTOR_SIZE)
typedef struct Begin_bk
{
    //------------contains the seg_no ----------
    //----------in begin bk---------------------
    u_int seg_no;

    //Note: Seg_sum_bk starts at the 2nd bk of seg
    Seg_sum_bk  * ssum_bk; 

}Begin_bk;

//----segment definition----------------
typedef struct Seg
{
    Begin_bk * begin_bk;
    Block * bk;

}Seg;

typedef struct LogAddress
{
    u_int seg_no;
    u_int bk_no;

}LogAddress;

//for cleaning policy,record segment usage table
//per segment linked to seg_usage_table
//检查每一个seg，知道其利用率
typedef struct Seg_usage_table
{
    u_int seg_no;

    //--------- bks that used now in the seg-------
    u_int num_live_bk;
    //u_int num_live_bytes; // should not be num of live blocks???
    
    time_t modify_time;

    struct Seg_usage_table * next;

}Seg_usage_table;


//contains direct block info in disk
typedef struct Block_pointer
{
    //------- file bk size in sectors--------
    //------actually in flash memory: 1 sector is its 1 block-------
    int seg_no;
    int bk_no;
}Block_pointer;


typedef struct Inode
{
    u_int ino;

    //    u_int seg_no_in_log;//this inode is stored in which seg of log
    //    u_int bk_no_in_log;//this inode is stored in which block of log
    u_int filetype;
    u_int filesize;
    char filename[FILE_NAME_LENGTH + 1]; //phase 1

    //flash address of the inode's blocks
    Block_pointer direct_bk[DIRECT_BK_NUM];
    //Bk_list indirect_bk;
    //Block_pointer indirect_bk;  phase 2

    mode_t mode;
    uid_t userID;
    gid_t groupID;
    time_t modify_Time;
    time_t access_Time;
    time_t create_Time;
    time_t change_Time;
    int num_links;

}Inode;



//--------1.addrs of all blocks in the inode map - ifile_no
//---- ifile contains this info------------------
//--------2.seg usage table
//--------3. current time
//--------4. pointer to the last segment written
typedef struct Checkpoint_region
{
    Inode * ifile;
    Seg_usage_table *seg_usage_table;
    u_int curr_time;
    Seg * last_seg_written;
}Checkpoint;




//super log segment 存整个log的信息和checkpoint等
typedef struct Super_seg
{
    u_int seg_no;
    u_int seg_num;      
    u_int seg_size;
    u_int bk_size;
 
    //----只记录除 super seg 外的data seg的利用率    
    Seg_usage_table * seg_usage_table;
    /* 
     * edited 1 weng 
     * checkpoint
     * Flash
     */
    //??checkpoint还没定义
    Checkpoint * checkpoint;

}Super_seg;


//大小以flash memory的seg为单位
typedef struct Disk_cache
{
    u_int cache_no;

    //this cache is from which seg
    Seg * seg;

    u_int IS_JUST_UPDATE;

    struct Disk_cache * next;

}Disk_cache;

//----------------gloabal value-----------------------
Super_seg * super_seg;
Disk_cache * disk_cache;

//-------points to the logAddress that could start to write data-------
LogAddress * tail_log_addr;

Seg * seg_in_memory;


u_int wearlimit;

//----flash memory name---------------
char * fl_file;          

//--------default: 1024-------------
//-------- chosen by user-------------
u_int sec_num;

//-------default: 2-----------------
//-------2 sectors = 1 block----------
u_int bk_size;

//------default: 32--------------------------
//---------chose by user-------------------
u_int bks_per_seg;

u_int seg_size;

//??注意应写程序保证若用户输入导致计算出的seg_num非整数则让用户重新输
//入
//-----------total seg num-------------
u_int seg_num;

//-----------一个bk 可以存多少bytes的数据-----------
//----------- in bytes-----------------------------
u_int bk_content_size;

//--------default :4 ------------------
//------chosen by user----------------
u_int cache_seg_num;   

//-------------------------------------------------------------




//-------------------method------------------------------------//


//--------create cache------------------------
//return 0: create successfully
//retrun 1: create not successfully
int create_cache();

//read_cache
//--return true all data is in cache, read directly from cache--------
//--return false not all data is in cache, read from disk,then---------
int read_cache(LogAddress  * logAddress, u_int length, void * buffer);


/*
 *logAddress indicates the log segment and block number within the segment, 
 *inum is the inode number of the file, 
 *block is the block number within the file,  
 *length is the number of bytes to read/write/free,  
 *buffer contains the data to read or write.
 */


//在flash memory里面分配几个segment，用来存log的结构
//调用flash.h的函数
int Log_Create();

//-----------------------------------------------------------
//input: disk 地址，返回长度为length的dis数据于buffer中
int Log_Read(LogAddress * log_addr, u_int length, void * buffer);


//-----------------------------------------------------------------
//将文件的inum(inode)的第block号块写入log,写入内容
//----------------------input--------------------------------------
//--------input: block - bk_no within the file
int Log_Write(u_int inum, u_int block, u_int length,
         void * buffer, LogAddress * log_addr);

//--------------------------------------------------------------------
//释放log中从logAddress开始长度为length的数据,
//--free 最小blocks数 which cover the length
int Log_Free(LogAddress * log_addr, u_int length);



int Log_Init(char* filename, Inode * ifile, u_int cachesize);
#endif
