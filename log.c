#include <stdio.h>
#include <stdlib.h>
#include "flash.h"
#include "log.h"
#include <string.h>

/*
 *
 *?? means not for sure or need improve in future
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
    u_int bks_per_seg,
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
    s_log_seg->log_seg_no = 0;
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
    Seg * data_start_seg;
    u_int i;
    for(i = 1; i < s_log_seg->log_seg_num; i++)
    {
        u_int j;
        Seg * log_seg = (Seg *)malloc(sizeof(Seg));
        //--------------------------start new log seg creation-------------
        //1.create new log seg
        log_seg->log_seg_no = i;

        //-------------------------------------------------------
        //1.1 create new log seg's Begin bk
        //for Seg_sum_bk's info

        Seg_sum_entry * entry = (Seg_sum_entry *)malloc(sizeof(Seg_sum_entry)); 
        entry->log_bk_no = 1; 
        entry->file_no = -1;
        entry->file_bk_no = -1;
        entry->next = NULL;

        Seg_sum_entry * start_entry = entry;
        
        for(j = 2; j < bks_per_seg; j++)
        {
            Seg_sum_entry * tmp_entry = (Seg_sum_entry *)malloc(sizeof(Seg_sum_entry)); 
            tmp_entry->log_bk_no = j; 
            tmp_entry->file_no = -1;
            tmp_entry->file_bk_no = -1;
            tmp_entry->next = NULL;

            while(entry->next != NULL)
                entry = entry->next;

            entry->next = tmp_entry;
        }
        

        Seg_sum_bk * seg_sum_bk = (Seg_sum_bk *)malloc(sizeof(Seg_sum_bk));
 //seg_sum_bk 在 1th block       
        seg_sum_bk->log_bk_no = 1;
        seg_sum_bk->seg_sum_entry = start_entry;

        Begin_bk * begin_bk = (Begin_bk *)malloc(sizeof(Begin_bk));
//??suppose begin_bk only has 1 seg_sum_bk 
        begin_bk->log_bk_no = start_entry->log_bk_no;
        begin_bk->ssum_bk = *(seg_sum_bk); 
        
        log_seg->begin_bk = *(begin_bk);        

        //----------------------------------------------------------
        //2.create the blocks for the seg
        Block * start_bk = (Block *)malloc(sizeof(Block));
        start_bk->bk_no = 1;
        void * start_bk_content = malloc(secs_per_bk * FLASH_SECTOR_SIZE);
        start_bk->bk_content = start_bk_content;
        start_bk->next = NULL;

        Block * copy_start_bk = start_bk;
        for(j = 2; j < bks_per_seg; j++)
        {
            Block * tmp_bk = (Block *)malloc(sizeof(Block));
            tmp_bk->bk_no = j;
            void * tmp_bk_content = malloc(secs_per_bk * FLASH_SECTOR_SIZE);
            tmp_bk->bk_content = tmp_bk_content;
            tmp_bk->next = NULL;
            
            while(start_bk->next != NULL)
                start_bk = start_bk->next;

            start_bk->next = tmp_bk;
        } 

        log_seg->bk = copy_start_bk;

        log_seg->next = NULL;
        //----------------finish log seg creation-----------------------------
        //-------------------------------------------------------------------

        //--------------attach new log seg to the end-------------------
        
        if(i == 1)
        {
             data_start_seg = log_seg;
             s_log_seg->next = log_seg;

        }
        else
        {
            while(data_start_seg->next != NULL)
                data_start_seg = data_start_seg->next;

            data_start_seg->next = log_seg;
        }
    }
    

}

