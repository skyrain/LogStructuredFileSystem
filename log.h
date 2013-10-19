#ifndef LOG_H
#define LOG_H


#include "flash.h"
#include <time.h>
#include <sys/types.h>

#define DIRECT_BK_NUM 4
#define FILE_NAME_LENGTH 8
#define BLOCK_SIZE FLASH_BLOCK_SIZE // temp for file layer test
//#define N_BEGIN_BLOCK 2


typedef struct Block
{
    u_int block_no;
    void *bk_content;   //存file data 
}Block;

//list "log_bk_no"th block in certain log seg
//it contaiins which file's which file block data
typedef struct Seg_sum_entry
{
    u_int log_bk_no;
    u_int file_no;            
    u_int file_bk_no;

}Seg_sum_entry;

//每个log seg有bks_per_seg个log blocks
typedef struct Seg_sum_bk
{
    u_int log_bk_no;        //该Seg_sum_bk存在log seg的哪一个block里面
    Seg_sum_entry * seg_sum_entry;

    //之后加入uid用于判断一个log seg的某个block是否alive
    //??

}Seg_sum_bk;

//Suppose only 1 Seg_sum_bk as Begin_bk in Seg
typedef struct Begin_bk
{
    u_int log_bk_no;        //Begin block在log seg的哪一个block里面
    Seg_sum_bk begin_block; 
}Begin_bk;

//log segment definition
typedef struct Seg
{
    u_int log_seg_no;
    Begin_bk begin_block;
    Block * blocks;
}Seg;

//for cleaning policy,record segment usage table
//per segment linked to seg_usage_table
//检查flash memory的每一个seg，知道其利用率
typedef struct Seg_usage_table
{
    u_int fl_seg_no;
    u_int num_live_bk;
    //u_int num_live_bytes; // should not be num of live blocks???
    time_t modify_Time;
}Seg_usage_table;


//store metadata
typedef struct Checkpoint_region
{
    //???address of all blocks in lfs or flash memory


}Checkpoint;

//contains direct block info in disk
//seg_no: which seg this block it belongs to
//bk_no: what block_no in the seg
typedef struct Block_pointer
{
    u_int seg_no;
    u_int bk_no;
}Block_pointer;

//this structure doesnt need
//typedef struct Bk_list
//{
//    Direct_bk bk;
//    struct Bk_list* next;
//}Bk_list;

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

//store the inode 
typedef struct Inode_location
{
    Inode inode;
    //offset in "ifile",
    // there stores the inode for ino
    u_int offset; 
    struct Inode_location * next;

}Inode_location;

//Ifile stores the inode_location of each inode
//in file "ifile"
typedef struct Ifile
{
    char * name;
    Inode_location * inode_loc;
}Ifile;


/*
typedef struct Inode_map_list
{
    Inode_location inode_location;
    struct Inode_map_list* next;
}Inode_map_list;

//inode_no list, stores the inode's id
//be written into log as block
typedef struct Inode_map
{
   Inode_map_list inode_map_list; 
}Inode_map;

*/

typedef struct LogAddress
{
    u_int log_seg_no;
    u_int log_bk_no;
}LogAddress;


//super log segment 存整个log的信息和checkpoint等
typedef struct Super_log_seg
{
    u_int log_seg_no;
    u_int seg_size;
    u_int bk_size;
    u_int fl_seg_num;                     //整个flash memory大小
    u_int log_seg_num;
    Seg_usage_table * seg_usage_table;
    /* 
     * edited 1 weng 
     * checkpoint
     * Flash
     */
    //??checkpoint还没定义
    Checkpoint * checkpoint;
    Ifile * ifile;

//    Flash *flash;

    //points to next log seg
    Seg *next;  

}Super_log_seg;


//大小以flash memory的seg为单位
typedef struct Disk_cache
{
    u_int fl_seg_size;  
    Seg disk_cache_seg;
    struct Disk_cache * next;
}Disk_cache;




#endif


/****
 *
 * method
 *
logAddress indicates the log segment and block number within the segment, 
inum is the inode number of the file, 
block is the block number within the file,  
length is the number of bytes to read/write/free,  
buffer contains the data to read or write.
 *******/

//在flash memory里面分配几个segment，用来存log的结构
//调用flash.h的函数
int Log_Create(
        char * file,
        u_int wearLimit,
        u_int total_sec,
        u_int sec_per_block,
        u_int bks_per_seg);

//input: disk 地址，返回长度为length的dis数据于buffer中
int Log_Read(u_int disk_seg,
        u_int disk_bk_no, u_int length, void * buffer);
   
//将文件的inum(inode)的第block号块写入log, 写入log的地址为logAddress,写入内容
int Log_Write(u_int inum, u_int block,u_int length,
         void * buffer, LogAddress * logAddress);
  
//释放log中从logAddress开始长度为length的数据
int Log_Free(LogAddress logAddress, u_int length);
    

