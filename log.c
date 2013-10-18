#include <stdio.h>
#include <stdlib.h>
#include "flash.h"
#include "log.h"
#include <string.h>

/*
 *
 *
 *
 *
 **********/


//wearlimit: 1000, seg:32 bk, b: 2 sector, total sect: 1024
//file: flash name, total_sec: flash size
//suppose: log layer's seg size and block size same as flash layer
int Log_Create(
    char * file,
    u_int wearLimit,
    u_int total_sec,
    u_int secs_per_bk,
    u_int bks_per_seg
    u_int segs_per_log
        )
{
    //initialize log structure,
    //根据用户输入参数创建flash memory
    //-------------------------------------------    
    u_int fl_bk_num = total_sec / secs_per_bk;
    //create flash file
    Flash_Create(file, wearLimit, total_sec);
    
    //format flash file as segment(same size as log seg)
    u_int log_seg_size = FLASH_SECTOR_SIZE * secs_per_bk * bks_per_seg;
  
    //1.store Super log seg in flash memory
    //1.1 create super log seg 
    u_int * s_log_seg_mem = (u_int *)malloc(log_seg_size);
    Super_log_seg * s_log_seg = (Super_log_seg *)s_log_seg_mem;
    s_log_seg->seg_size = log_seg_size;
    s_log_seg->bk_size = secs_per_bk * FLASH_SECTOR_SIZE;
    s_log_seg->fl_seg_num = total_sec / secs_per_bk / bks_per_seg;
    s_log_seg->log_seg_num = segs_per_log;

    Seg_usage_table* seg_usage_table = (Seg_usage_table *)malloc(sizeof(Seg_usage_table) * s_log_seg->fl_seg_num);
    s_log_seg->seg_usage_table = seg_usage_table; 

    Checkpoint * checkpoint = (Checkpoint *)malloc(sizeof(Checkpoint));
    s_log_seg->checkpoint = checkpoint;
    Ifile * ifile = (Ifile *)malloc(sizeof(Ifile));
    ifile->name = (char *)malloc(4);
    strcpy(ifile->name, "ifile");
    s_log_seg->ifile = ifile;
    s_log_seg->next = NULL;
    

    
    //1.2 put super log seg in disk (start from disk first sector)
    //choose the model of Flashy
    Flash_Flags flags = FLASH_SILENT;
    
    //blocks : # of blocks in the flash
    u_int tmp = bks_per_seg * s_log_seg->fl_seg_num;
    u_int * blocks = &tmp;
    Flash   flash = Flash_Open(file, flags, blocks); 
    
    Flash_Write(flash, 0, secs_per_bk * bks_per_seg, s_log_seg);    
    
    //2.create other log seg in memory, and attached to super log seg
    u_int i;
    for(i = 0; i < s_log_seg->log_seg_num; i++)
    {
        //create new log seg
        u_int * tmp_log_seg = (u_int *)malloc(log_seg_size);
        Seg * log_seg = (Seg *)tmp_log_seg;
        log_seg->
//.............


        s_log_seg->next = log_seg;


    }
    

}

