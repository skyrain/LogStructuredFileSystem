#ifndef LOG_H
#define LOG_H


#include "flash.h"
#include <time.h>
#include <sys/types.h>

#define BLOCK_SIZE FLASH_BLOCK_SIZE
#define SEG_SIZE (BLOCKS_PER_SEG * BLOCK_SIZE)
#define DIRECT_BK_NUM 4
#define FILE_NAME_LENGTH 8
//#define N_BEGIN_BLOCK 2

typedef struct Block
{
    u_int block_size = BLOCK_SIZE;
    u_int block_no;
}Block;

//store info of which block used by which file.
typedef struct Seg_sum_entry
{
    u_int file_no;              //should it be inode??
    u_int block_no;
}Seg_sum_entry;

//Seg_num consists of Seg_sum_entry
typedef struct Seg_sum_bk
{
    u_int block_no;
    Seg_sum_entry* seg_sum_entry;
}Seg_sum_bk;

//Suppose only 1 Seg_sum_bk as Begin_bk in Seg
typedef struct Begin_bk
{
    u_int block_no;
    Seg_sum_bk* begin_block;
} Begin_bk;

//segment definition
struct Seg
{
    Begin_bk begin_block;
    u_int seg_size = SEG_SIZE;
    u_int seg_no;
};

//for cleaning policy,record segment usage table
//per segment linked to seg_usage_table
typedef struct Seg_usage_table
{
    u_int seg_no;
    u_int num_live_bk;
    //u_int num_live_bytes; // should not be num of live blocks???
    time_t modify_Time;
} Seg_usage_t;


//store metadata
typedef struct Checkpoint_region
{
    //???address of all blocks in lfs or flash memory


}Checkpoint;
//contains direct block info
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
    u_int seg_no_in_log;//this inode is stored in which seg of log
    u_int bk_no_in_log;//this inode is stored in which block of log
    u_int filetype;
    u_int filesize;
    char filename[FILE_NAME_LENGTH+1]; //phase 1
    //flash address of the inode's blocks
    Block_pointer direct_bk[DIRECT_BK_NUM];
    //Bk_list indirect_bk;
    //Block_pointer indirect_bk;  phase 2
    int mode;
    uid_t userID;
    gid_t groupID;
    time_t modify_Time;
    time_t accese_Time;
    time_t create_Time;
    time_t change_Time;
    int num_links;
}Inode;

//store the inode and its corresponding location in log
typedef struct Inode_location
{
    u_int ino;
    u_int log_seg_no;
    u_int log_bk_no;
}Inode_location;

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
};

struct log_beginning
{
    u_int segment_size = FLASH_BLOCK_SIZE * BLOCKS_PER_SEGMENT;
    u_int block_size = FLASH_BLOCK_SIZE;
    //how many segments in flash memory
    //????????????????????????????
    u_int SEG_NUM = ;
    seg_usage_table[SEG_NUM] seg_usage_table;
    Inode_map Inode_map;
    //???anything else
    //???anything else
    /* 
     * edited 1 weng 
     * checkpoint
     * Flash
     */
     Checkpoint checkpoint;
     Flash *flash;
};

#endif




