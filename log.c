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
int Log_Create()
{
    u_int i;
    //--------------------------------------------------------------
    //-----1st log seg is super log seg----------------------------
    //-------data seg start from 1 ---------------------------
    //-----------[1, segs per log -1]----------------
    tail_log_addr = (LogAddress *)calloc(1, sizeof(LogAddress));
    tail_log_addr->seg_no = 1;
    //-----------[1, bks per seg - 1]-----------
    tail_log_addr->bk_no = 1;

    //initialize log structure,
    //根据用户输入参数创建flash memory
    //-------------------------------------------    
    //create flash file
    Flash_Create(fl_file, wearlimit, sec_num);
    
    //----------------- super  seg      ----------------------------
    //---------------------------------------------------------------
    //format flash file as segment(same size as log seg)
    u_int seg_size_bytes = FLASH_SECTOR_SIZE * seg_size;
  
    //1.store Super log seg in flash memory's 1st seg
    //1.1 create super log seg 
    
    //----about calloc----------
    Super_seg * s_seg = (Super_seg *)calloc(1, sizeof(Super_seg));

    super_seg = s_seg;

    s_seg->seg_no = 0;
    s_seg->seg_num = seg_num;
    s_seg->seg_size = seg_size;
    s_seg->bk_size = bk_size;

    Seg_usage_table * start_sut = (Seg_usage_table *)calloc(1, sizeof(Seg_usage_table));
    start_sut->seg_no = 1;
    start_sut->num_live_bk = 0;
    start_sut->modify_time = -1;
    start_sut->next = NULL;

    s_seg->seg_usage_table = start_sut;

    Seg_usage_table * sut_walker = start_sut;
    for(i = 2; i < seg_num; i++)
    {
        Seg_usage_table * tmp_sut = (Seg_usage_table *)calloc(1, sizeof(Seg_usage_table));
        tmp_sut->seg_no = i;
        tmp_sut->num_live_bk = 0;
        tmp_sut->modify_time = -1;

        while(sut_walker->next != NULL)
           sut_walker = sut_walker->next;

        sut_walker->next = tmp_sut; 
    }
    
    Checkpoint * checkpoint = (Checkpoint *)calloc(1, sizeof(Checkpoint));
   
    s_seg->checkpoint = checkpoint;

    checkpoint->ifile_ino = -1;
    
    Seg_usage_table * s_sut = (Seg_usage_table *)calloc(1, sizeof(Seg_usage_table));
    s_sut->seg_no = 1;
    s_sut->num_live_bk = 0;
    s_sut->modify_time = -1;
    s_sut->next = NULL;

    checkpoint->seg_usage_table = s_sut;

    sut_walker = s_sut;
    for(i = 2; i < seg_num; i++)
    {
        Seg_usage_table * tmp_sut = (Seg_usage_table *)calloc(1, sizeof(Seg_usage_table));
        tmp_sut->seg_no = i;
        tmp_sut->num_live_bk = 0;
        tmp_sut->modify_time = -1;

        while(sut_walker->next != NULL)
           sut_walker = sut_walker->next;

        sut_walker->next = tmp_sut; 
    }
 
    checkpoint->curr_time = -1;
    checkpoint->last_seg_written = NULL;

    
    //1.2 put super log seg in disk (start from disk first sector)
    //choose the model of Flash
    Flash_Flags flags = FLASH_SILENT;
    
    //blocks : # of blocks in the flash
    u_int tmp = sec_num / 16;
    u_int * blocks = &tmp;
    Flash   flash = Flash_Open(fl_file, flags, blocks); 
    
    Flash_Write(flash, 0, seg_size, s_seg);    
    Flash_Close(flash);
    //-----------------------------------------------------------


    //----------------------------------------------------------------
    //------------create normal seg-----------------------------------
    //---------------------------------------------------------------
    for(i = 1; i < seg_num; i++)
    {
//        Seg * tmp = (Seg *)calloc(1, sizeof(Seg));

        Begin_bk * tmp_bb = (Begin_bk *)calloc(1, sizeof(Begin_bk));

//        tmp->begin_bk = tmp_bb;
        //--------begin bk----------------------------------
        Seg_sum_bk * ssum = (Seg_sum_bk *)calloc(1, sizeof(Seg_sum_bk));
        tmp_bb->ssum_bk = ssum;
        ssum->bk_no = 0;

        Seg_sum_entry * s_entry = (Seg_sum_entry *)calloc(1, sizeof(Seg_sum_entry));

        ssum->seg_sum_entry = s_entry;

        s_entry->bk_no = 1;
        s_entry->file_no = -1;
        s_entry->file_bk_no = -1;
        s_entry->next = NULL;

        Seg_sum_entry * entry_walker = s_entry;

        u_int j;
        for(j = 2; j < bks_per_seg; j++)
        {
            Seg_sum_entry * t_entry = (Seg_sum_entry *)calloc(1, sizeof(Seg_sum_entry));
            t_entry->bk_no = j;
            t_entry->file_no = -1;
            t_entry->file_bk_no = -1;

            while(entry_walker->next != NULL)
                entry_walker = entry_walker->next;

            entry_walker->next = t_entry;
        }
       
        //-------write begin bk in disk in block size---------
        flash = Flash_Open(fl_file, flags, blocks); 
        u_int s_sec = seg_size * FLASH_SECTOR_SIZE * i;
        Flash_Write(flash, s_sec, bk_size, tmp_bb);    
        Flash_Close(flash);

        //-------------normal bk-----------------------------
        //---start offset: seg_start + bk_size * FLASH_SECTOR_SIZE
        for(j = 1; j < bks_per_seg; j++)
        {
            Block * tmp_b = (Block *)calloc(1, sizeof(Block));
            flash = Flash_Open(fl_file, flags, blocks); 
            u_int s_sec = seg_size * FLASH_SECTOR_SIZE * i + bk_size * j;
            Flash_Write(flash, s_sec, bk_size, tmp_b);    
            Flash_Close(flash);
        }            
    }
    
    return 0;
}

/*


//---------------create cache---------------------------------------
//---------------input: fl_seg_size: in sectors--------------
int create_cache(u_int seg_num, u_int fl_seg_size)
{
    Disk_cache *cache_start = (Disk_cache *)calloc(sizeof(Disk_cache));
    cache_start->cache_no = 0;
    cache_start->fl_seg_no = -1;
    cache_start->content = calloc(fl_seg_size * FLASH_SECTOR_SIZE);
    cache_start->IS_JUST_UPDATE = false;
    cache_start->next = NULL;

    disk_cache = cache_start;

    u_int i;
    for(i = 1; i < seg_num; i++)
    {
        Disk_cache * tmp = (Disk_cache *)calloc(sizeof(Disk_cache));
        tmp->cache_no = i;
        tmp->fl_seg_no = -1;
        tmp->content = calloc(fl_seg_size * FLASH_SECTOR_SIZE);
        tmp->IS_JUST_UPDATE = false;
        tmp->next = NULL;

        while(cache_start->next != NULL)
            cache_start = cache_start->next;

        cache_start->next = tmp; 
    }
    return 0;   
}


//-----------------read_cache----------------------------------
//--- ??  input: length in bytes
//------- input: buffer only need to be a pointer
//--------length  = the size of block--------------------------
bool read_cache(Disk_addr disk_addr, u_int length, void * buffer)
{
    buffer = calloc(length);
    bool IS_IN_CACHE = false;

    //blocks remain in first seg
    u_int bks_remain = fl_bks_per_seg - disk_addr.fl_bk_no;
  
    u_int bks_tobe_read = length / FLASH_SECTORS_SIZE / fl_secs_per_bk;
    
    u_int bytes_filled = 0;

    Disk_cache cache_walker = disk_cache;
    while(cache_walker != NULL)
    {
        //If the input disk_addr's seg no is in the cache
        if(disk_addr->fl_seg_no == cache_walker->fl_seg_no)
        {
            //write data small than data size of bks_remain
            memcpy(buffer, cache_walker->content
                    + disk_addr->fl_bk_no * fl_bk_size, 
                    length);

            IS_IN_CACHE = true;
            break;
        }
        
        cache_walker= cache_walker->next;

    }
   
    if(!IS_IN_CACHE)
        free(buffer);

    return IS_IN_CACHE;
}

//----------------------------------Note-------------------------
//-------access flash memory should always in terms of seg-------



//input: disk 地址，返回长度为length的dis数据于buffer中
//Here now: length is always = fl_bk_size
//--------------------不读 Begin block----------------------------
int Log_Read(Disk_addr disk_addr, u_int length, void * buffer);
{
    //attemp to read from cache
    bool IS_IN_CACHE = read_cache(disk_addr, length, buffer)    
    if(IS_IN_CACHE)
        return 0;

    //If not in cache, read data from flash, 
    //then store it on cache from the 1st cache seg
    //------------1. read data from flash-----------
    //choose the model of Flash
    Flash_Flags flags = FLASH_SILENT;
    
    u_int tmp = fl_bks_per_seg * super_log_seg->fl_seg_num;
    u_int * blocks = &tmp;
    Flash flash = Flash_Open(file, flags, blocks); 
    
    u_int sec_offset = disk_addr->fl_seg_no * fl_seg_size;

    //??need calloc before use buffer to this func? 
    void * new_buffer;// = malloc(fl_seg_size * FLASH_SECTOR_SIZE);
    
    //Read the whole seg which contains the data(only 1 bk size)
    Flash_Read(flash, sec_offset, fl_seg_size, new_buffer);
    
    Flash_Close(flash);

    buffer = calloc(fl_bk_size);
    memcpy(buffer, new_buffer + disk_addr->fl_bk_no * fl_secs_per_bk, 
            fl_bk_size * FLASH_SECTOR_SIZE);
     
    //----------2. store on cache-----------------
    //-------------2.1 check whether all are just updated----
    //---if all yes, set each's IS_JUST_UPDATE = false;
    bool ALL_UPDATED = true;
    Disk_cache  * c_walker = disk_cache;
    while(c_walker != NULL)
    {
        if(!c_walker->IS_JUST_UPDATE)
        {
            ALL_UPDATED = false;
            break;
        }
        c_walker = c_walker->next;
    }
    
    c_walker = disk_cache;
    if(ALL_UPDATED)
    {
        while(c_walker != NULL)
        {
            c_walker->IS_JUST_UPDATE = false;
            c_walker = c_walker->next;
        }

    }    
    //------------2.2 choose 1st cache seg whose-------------
    //-------IS_JUST_UPDATE = false, then memcpy b--------------------
    c_walker = disk_cache;
    while(c_walker != NULL)
    {
        if(!c_walker->IS_JUST_UODATED)
        {
            c_walker->fl_seg_no = disk_addr->fl_seg_no;

            memcpy(c_walker->content, new_buffer, 
                    fl_seg_size * FLASH_SECTOR_SIZE);
            free(new_buffer);
            
            c_walker->IS_JUST_UPDATE = true;
        }
    }

    return 0;

}

//-------------assist func--------------------------------------

//--------call this func after write data to log----------------
void setLogTail()
{
    //If come to end of certain log seg
    if(tail_log_addr->log_bk_no == log_bks_per_seg -1)
    {
        //If come to every end of log structure
        if(tail_log_addr->log_seg_no == segs_per_log - 1)
        {
            tail_log_addr->log_seg_no = 1;
            tail_log_addr->log_bk_no = 1;
        }
        //else turn to next log seg's 1th bk
        else
        {
            tail_log_addr->log_seg_no += 1;
            tail_log_addr->log_bk_no = 1;
        }
    }
    //just add 1 of bk_no
    else
    {
        tail_log_addr->log_bk_no += 1;
    }
}


//---------once tail_log_addr reaches certain log seg's end--------
//-------- push that log seg data into disk------------------------
//---------call this func before setLogTail()-------------------
void pushToDisk()
{
    //If reaches certain log seg's end, write entire log seg to disk
    if(tail_log_addr->log_bk_no == log_bks_per_seg -1)
    {
        //1.locate the full log seg
        Seg * log_seg_walker = super_log_seg->next;
        while(log_seg_walker != NULL)
        {
            if(tail_log_seg_addr->log_seg_no == log_seg_walker->log_seg_no)
                break;

            log_seg_walker = log_seg_walker->next;
        }   
        
        //2.write to disk
        //choose the model of Flash
        Flash_Flags flags = FLASH_SILENT;

        //blocks : # of blocks in the flash
        u_int tmp = fl_bks_per_seg * super_log_seg->fl_seg_num;

        u_int * blocks = &tmp;
        Flash   flash = Flash_Open(file, flags, blocks);


                
        u_int secs_per_bk = super_log_seg->bk_size;  
        //write Begin block into disk
        void * begin_buffer = &(log_seg_walker->begin_bk);

        Flash_write(flash, log_seg_walker->log_seg_no * log_seg_size,
                secs_per_bk, begin_buffer);
        
        Log_Block * bk_walker = log_seg_walker->bk;
        //write data block into disk
        while(bk_walker != NULL)
        {
            void * bk_buffer = bk_walker->bk_content;


            u_int sec_offset = log_seg_walker->log_seg_no * log_seg_size 
                + bk_walker->log_bk_no *  secs_per_bk;

            Flash_Write(flash, sec_offset, secs_per_bk, bk_buffer);  
            
            bk_walker = bk_walker->next;
        } 
        Flash_Close(flash);
    }
}

//-------------------write data to log seg's one block---------------
void writeToLog()
{

}



//------------------------------------------------------------------




//-----------------------------------------------------------------
//将文件的inum(inode)的第block号块写入log, 写入log的地址为tail_log_addr,写入内i容
//----------------------input--------------------------------------
//--------input: length - always = fl_bk_size;
//-----------!!! 一次只写一个 block的数据--------------------------
//--------input: block - bk no within the file
int Log_Write(u_int inum, u_int block, u_int length,
                 void * buffer, Disk_addr * disk_addr)
{

    //3. change inode content
    //4. change seg usage table
    //5. change sector wearlimt
    //6. change


}

*/
