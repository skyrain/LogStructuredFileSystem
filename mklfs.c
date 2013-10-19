#include <stdio.h>
#include <stdlib.h>
#include "flash.h"
#include "log.h"

//--------------------global variables----------------------
u_int fl_wearlimit;
char * fl_file;          //flash memory name

u_int fl_sec_num;
u_int fl_secs_per_bk;   //？？与FLASH_SECTORS_PER_BLOCK关系
u_int fl_bks_per_seg;
u_int fl_bk_size;
u_int fl_seg_size;
// 1024个sector, 2个sector组成1block,32个block组成一个seg
//??注意应写程序保证若用户输入导致计算出的fl_seg_num非整数则让用户重新输入
u_int fl_seg_num;
u_int cache_seg_num;   //specify by the user


//根据用户输入定义log的相应结构大小
u_int log_bk_size;
u_int log_bks_per_seg;
u_int segs_per_log;  //程序员定义
u_int log_seg_size;

//cache seg 数量，此处为默认值
u_int cache_seg_num; 

//-------------------------------------------------------------



int main(int argc, char *argv[])
{

//用户输入--------------------------------------------    
    //？？之后会作为参数设置,此处为默认值
    //用户输入
    fl_wearlimit = 1000;
    strcpy(fl_file, "File");          //flash memory name

    fl_sec_num = 1024;
    fl_secs_per_bk = 16;   //？？与FLASH_SECTORS_PER_BLOCK关系
    fl_bks_per_seg = 32;
    
    //size in bytes
    fl_bk_size = fl_secs_per_bk * FLASH_SECTOR_SIZE;
    
    //size in sectors
    fl_seg_size = fl_secs_per_bk * fl_bks_per_seg;
    // 1024个sector, 2个sector组成1block,32个block组成一个seg
    //??注意应写程序保证若用户输入导致计算出的fl_seg_num非整数则让用户重新输入
    fl_seg_num = fl_sec_num / fl_secs_per_bk / fl_bks_per_seg;
    cache_seg_num = 2;   //specify by the user
        
        
        

    //根据用户输入定义log的相应结构大小
   
    //size in bytes
    log_bk_size = fl_secs_per_bk * FLASH_SECTOR_SIZE;
    
    log_bks_per_seg = fl_bks_per_seg;
    segs_per_log = 4;  //程序员定义
    log_seg_size = log_bks_per_seg * log_bk_size;

    //cache seg 数量，此处为默认值
    cache_seg_num = 4; 
 //用户输入--------------------------------------------------


     


    //create and format flash memory & create log in memory
    Log_Create(fl_file,fl_wearlimit,
            fl_sec_num, fl_secs_per_bk, fl_bks_per_seg, segs_per_log);

    //create cache: store in dis_cache 
    create_cache(cache_seg_num, fl_seg_size);


}
