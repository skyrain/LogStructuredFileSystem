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
    //initialize global variable
    //--------------------------------------------------------------
    //-----1st log seg is super log seg----------------------------
    //-------usable log seg start from 1 ---------------------------
    tail_log_seg->log_seg_no = 1;
    tail_log_seg->log_bk_no = 0;

    //initialize log structure,
    //根据用户输入参数创建flash memory
    //-------------------------------------------    
    u_int fl_bk_num = total_sec / secs_per_bk;
    //create flash file
    Flash_Create(file, wearLimit, total_sec);
    
    //format flash file as segment(same size as log seg)
    u_int log_seg_size = FLASH_SECTOR_SIZE * secs_per_bk * bks_per_seg;
  
    //1.store Super log seg in flash memory's 1st fl_seg
    //1.1 create super log seg 
    u_int * s_log_seg_mem = (u_int *)calloc(log_seg_size);
    Super_log_seg * s_log_seg = (Super_log_seg *)s_log_seg_mem;

    //----------------------------------------------
    //assign to global value super_log_seg
    super_log_seg = s_log_seg;

    s_log_seg->log_seg_no = 0;
    s_log_seg->seg_size = log_seg_size;
    s_log_seg->bk_size = secs_per_bk * FLASH_SECTOR_SIZE;
    s_log_seg->fl_seg_num = total_sec / secs_per_bk / bks_per_seg;
    s_log_seg->log_seg_num = segs_per_log;

    Seg_usage_table* seg_usage_table = (Seg_usage_table *)calloc(sizeof(Seg_usage_table) * s_log_seg->fl_seg_num);
    s_log_seg->seg_usage_table = seg_usage_table; 

    Checkpoint * checkpoint = (Checkpoint *)calloc(sizeof(Checkpoint));
    s_log_seg->checkpoint = checkpoint;
    Ifile * ifile = (Ifile *)calloc(sizeof(Ifile));
    ifile->name = (char *)calloc(4);
    strcpy(ifile->name, "ifile");
    s_log_seg->ifile = ifile;
    s_log_seg->next = NULL;
    

    
    //1.2 put super log seg in disk (start from disk first sector)
    //choose the model of Flash
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
        Seg * log_seg = (Seg *)calloc(sizeof(Seg));
        //--------------------------start new log seg creation-------------
        //1.create new log seg
        log_seg->log_seg_no = i;

        //-------------------------------------------------------
        //1.1 create new log seg's Begin bk
        //for Seg_sum_bk's info

        Seg_sum_entry * entry = (Seg_sum_entry *)calloc(sizeof(Seg_sum_entry)); 
        entry->log_bk_no = 1; 
        entry->file_no = -1;
        entry->file_bk_no = -1;
        entry->next = NULL;

        Seg_sum_entry * start_entry = entry;
        
        for(j = 2; j < bks_per_seg; j++)
        {
            Seg_sum_entry * tmp_entry = (Seg_sum_entry *)calloc(sizeof(Seg_sum_entry)); 
            tmp_entry->log_bk_no = j; 
            tmp_entry->file_no = -1;
            tmp_entry->file_bk_no = -1;
            tmp_entry->next = NULL;

            while(entry->next != NULL)
                entry = entry->next;

            entry->next = tmp_entry;
        }
        
        
        Seg_sum_bk * seg_sum_bk = (Seg_sum_bk *)calloc(sizeof(Seg_sum_bk));
 //seg_sum_bk 在 1th block       
        seg_sum_bk->log_bk_no = 0;
        seg_sum_bk->seg_sum_entry = start_entry;

        Begin_bk * begin_bk = (Begin_bk *)calloc(sizeof(Begin_bk));
//??suppose begin_bk only has 1 seg_sum_bk 
        begin_bk->log_bk_no = 0;
        begin_bk->ssum_bk = *(seg_sum_bk); 
        
        log_seg->begin_bk = *(begin_bk);        

        //----------------------------------------------------------
        //2.create the blocks for the seg
        Log_Block * start_bk = (Log_Block *)calloc(sizeof(Log_Block));
        start_bk->log_bk_no = 1;
        void * start_bk_content = calloc(secs_per_bk * FLASH_SECTOR_SIZE);
        start_bk->log_bk_content = start_bk_content;
        start_bk->next = NULL;

        Log_Block * copy_start_bk = start_bk;
        for(j = 2; j < bks_per_seg; j++)
        {
            Log_Block * tmp_bk = (Log_Block *)calloc(sizeof(Log_Block));
            tmp_bk->log_bk_no = j;
            void * tmp_bk_content = calloc(secs_per_bk * FLASH_SECTOR_SIZE);
            tmp_bk->log_bk_content = tmp_bk_content;
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

    
    //--------------------format flash memory----------------
    //-----1st fl seg is stored for super_log_seg-----------
    //------data stores from 2nd fl seg-------------------
    //------fl_seg info just stored in memory for manipulation easy---   

    //---initialize fl bk ------------------------------
    i = 1; // fl_seg no
    u_int j;//fl_bk no
    u_int fl_seg_num = total_sec / secs_per_bk / bks_per_seg; 
    Fl_Seg * fl_seg_walker = fl_seg;
    for(; i < fl_seg_num; i++)
    {

        Fl_Seg * tmp_fl_seg = (Fl_Seg *)malloc(sizeof(Fl_Seg));

        Fl_BLock * start_bk = (FL_Block *)malloc(sizeof(Fl_Block));
        start_bk->fl_bk_no = 0;
        start_bk->ino = -1;
        start_bk->in_bk_no = -1;
        start_bk->next = NULL;

        Fl_Block * bk_walker = start_bk;
        for(j = 1; j < bks_per_seg; j++)
        {
            Fl_Block * tmp_bk = (Fl_Block *)malloc(sizeof(Fl_Block));
            tmp_bk->fl_bk_no = j;
            tmp_bk->ino = -1;
            tmp_bk->ino_bk_no = -1;
            tmp_bk->next = NULL;

            while(bk_walker != NULL)
                bk_walker= bk_walker->next;

            bk_walker->next = tmp_bk;
        }
        
        if(i == 1)
        {
            fl_seg->fl_seg_no = 1;
            fl_seg->fl_bk = start_bk;
            fl_seg->next = NULL;
        }
        else
        {
            tmp_fl_seg->fl_seg_no = i;
            tmp_fl_seg->fl_bk = start_bk;
            tmp_fl_seg->next = NULL;

            while(fl_seg_walker->next != NULL)
                fl_seg_walker = fl_seg_walker->next;

            fl_seg_walker->next = tmp_fl_seg; 
        }
    }
    
    return 0;
}


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
    //    + disk_addr->fl_bk_no * fl_secs_per_bk;

    //??need calloc before use buffer to this func? 
    void * new_buffer;// = malloc(fl_seg_size * FLASH_SECTOR_SIZE);
    
    //Read the whole seg which contains the data(only 1 bk size)
    Flash_Read(flash, sec_offset, fl_seg_size, new_buffer);
    
    buffer = malloc(fl_bk_size);
    memcpy(buffer, new_buffer 
            + disk_addr->fl_bk_no * fl_secs_per_bk, fl_bk_size);
    
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



//将文件的inum(inode)的第block号块写入log, 写入log的地址为logAddress,写入内容
int Log_Write(u_int inum, u_int block, u_int length,
                 void * buffer, Disk_addr * disk_addr)
{
    //---------------- 


}
