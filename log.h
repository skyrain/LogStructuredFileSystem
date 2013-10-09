#include "flash.h"a
#include <time.h>
#include <sys/types.h>

#define BLOCKS_PER_SEG 16
#define BLOCK_SIZE FLASH_BLOCK_SIZE
#define SEG_SIZE (BLOCKS_PER_SEG * BLOCK_SIZE)
//#define N_BEGIN_BLOCK 2

struct Block
{
    u_int block_size = BLOCK_SIZE;
    u_int block_no;
}

typedef struct Block_Usage
{
	int file_no;
	int block_no;
}Block_Usage; //store the info about is that block being used by which file.

typedef struct Seg_sum_entry
{
    u_int file_no;              //should it be inode??
    u_int block_no;
}Seg_sum_entry;

typedef struct Seg_sum
{
    u_int block_no;
    Block_Usage *BlockUsageTable;
    Seg_sum_entry* seg_sum_entry
        = (Seg_sum_entry* )malloc(sizeof(Seg_sum_entry) * BLOCKS_PER_SEG);    
}Seg_sum;

typedef struct Begin_block
{
    u_int block_no;
    Seg_num* begin_block = (Seg_num* )malloc(sizeof(Seg_num));
}Begin_block;

typedef struct Segment
{
    Begin_block begin_block;
    u_int seg_size = SEGMENT_SIZE;
    u_int seg_no;
    //////////
}


struct seg_usage_table
{
    u_int NumLiveBlocks;
    //u_int num_live_bytes; // should not be num of live blocks???
    time_t modify_Time;
}SegUsagT

struct checkpoint_region
{
    
}

typedef struct Inode
{
	int  ino;
	int filesize;
	int mode;
	uid_t userID;
	gid_t groupID;
	time_t modify_Time;
	time_t accese_Time;
	time_t create_Time;
	time_t change_Time;
	int num_links;
}inode;

struct log_beginning
{
    u_int segment_size = FLASH_BLOCK_SIZE * BLOCKS_PER_SEGMENT;
    u_int block_size = FLASH_BLOCK_SIZE;
    struct seg_usage_table seg_usage_table; 
    
}
