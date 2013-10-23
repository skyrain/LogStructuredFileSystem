#include <stdio.h>
#include <stdlib.h>
#include "flash.h"
#include "log.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

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
    Flash_Create(fl_file, wearlimit, sec_num / FLASH_SECTORS_PER_BLOCK);
    
    //----------------- super  seg      ----------------------------
    //---------------------------------------------------------------
    //format flash file as segment(same size as log seg)
//    u_int seg_size_bytes = FLASH_SECTOR_SIZE * seg_size;
  
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


    Inode * tmp_inode = (Inode *)calloc(1, sizeof(Inode));

    tmp_inode->ino = -1;
    tmp_inode->filetype = 0;
    tmp_inode->filesize = 0;
    strcpy(tmp_inode->filename, "ifile");
    for(i = 0; i < DIRECT_BK_NUM; i++)
    {
        tmp_inode->direct_bk[i].seg_no = -1;
        tmp_inode->direct_bk[i].bk_no = -1;
    }
    tmp_inode->mode = 0;
    tmp_inode->userID = getuid();
    tmp_inode->groupID = getgid();
    time_t t;
    time(&t);
    tmp_inode->modify_Time = t;
    tmp_inode->access_Time = t;
    tmp_inode->create_Time = t;
    tmp_inode->change_Time = t;
    tmp_inode->num_links = 0;

    checkpoint->ifile = tmp_inode;
    




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

        Begin_bk * tmp_bb = (Begin_bk *)calloc(1, sizeof(Begin_bk));

        //--------begin bk----------------------------------
        tmp_bb->seg_no = i;

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
        u_int s_sec = seg_size * i;
        Flash_Write(flash, s_sec, bk_size, tmp_bb);    
        Flash_Close(flash);

        //-------------normal bk-----------------------------
        //---start offset: seg_start + bk_size * FLASH_SECTOR_SIZE
        for(j = 1; j < bks_per_seg; j++)
        {
            Block * tmp_b = (Block *)calloc(1, sizeof(Block));
            flash = Flash_Open(fl_file, flags, blocks); 
            s_sec = seg_size * i + bk_size * j;
            Flash_Write(flash, s_sec, bk_size, tmp_b);    
            Flash_Close(flash);
        }            
    }
    
    return 0;
}

//---------------create cache---------------------------------------
int create_cache()
{
    Disk_cache *cache_start = (Disk_cache *)calloc(1, sizeof(Disk_cache));
    cache_start->cache_no = 0;
    cache_start->seg = (Seg *)calloc(1, sizeof(Seg));
    cache_start->IS_JUST_UPDATE = false;
    cache_start->next = NULL;

    disk_cache = cache_start;

    u_int i;
    for(i = 1; i < cache_seg_num; i++)
    {
        Disk_cache * tmp = (Disk_cache *)calloc(1, sizeof(Disk_cache));
        tmp->cache_no = i;
        tmp->seg = (Seg *)calloc(1, sizeof(Seg));
        tmp->IS_JUST_UPDATE = false;
        tmp->next = NULL;

        while(cache_start->next != NULL)
            cache_start = cache_start->next;

        cache_start->next = tmp; 
    }
    return 0;   
}

//----------help identify the minimum # of bks --------------
//-------- for the length(in bytes)--------------------------
u_int length_in_bk(u_int len) 
{
    u_int bks_count = 0;
    while(bks_count * bk_size * FLASH_SECTOR_SIZE <= len)
    {
        bks_count++;
    }

    return bks_count;
}



//-----------------read_cache----------------------------------
//------- input: buffer only need to be a pointer
//--------- now file layer only give me length = 1 block data in bytes--------
//---------for file layer simplicity--------------------------------------
//-------- now this func can deal with length could be > 1 block data in bytes------------
bool read_cache(LogAddress * log_addr, u_int length, void * buffer)
{
    buffer = calloc(1, length);
    u_int offset = 0;

    bool IS_IN_CACHE = false;

    //blocks remain in 1st seg
    u_int bks_remain = bks_per_seg - log_addr->bk_no;
 
    u_int bks_tobe_read = length_in_bk(length);
    
    Disk_cache * cache_walker = disk_cache;

    while(cache_walker != NULL)
    {
        //If the input log_addr's seg no is in the cache
        if(log_addr->seg_no == cache_walker->seg->begin_bk->seg_no)
        {
            //---if all data stored in the same seg--------------
            if(bks_tobe_read <= bks_remain)
            {
                //write data smaller than data size of bks_remain
                memcpy(buffer + offset, cache_walker->seg->begin_bk
                        + log_addr->bk_no * bk_size * FLASH_SECTOR_SIZE, length);
                IS_IN_CACHE = true;
                break;
            }
            else
            {
                memcpy(buffer + offset, cache_walker->seg->begin_bk
                        + log_addr->bk_no * bk_size * FLASH_SECTOR_SIZE,
                        bks_remain * bk_size * FLASH_SECTOR_SIZE);
                offset += bks_remain * bk_size * FLASH_SECTOR_SIZE;
                length = length - bks_remain * bk_size * FLASH_SECTOR_SIZE;
                bks_remain = bks_per_seg;
                bks_tobe_read = length_in_bk(length);
                log_addr->seg_no++;
                log_addr->bk_no = 1;
                cache_walker = disk_cache;
            }

        }
        else
            cache_walker= cache_walker->next;
    }
   
    if(!IS_IN_CACHE)
        free(buffer);

    return IS_IN_CACHE;
}

//----------------------------------Note-------------------------
//-------access flash memory should always in terms of seg-------

//----------help identify the minimum # of segs --------------
//-------- for the length(in bks)--------------------------
u_int length_in_seg(u_int bks_tobe_read) 
{
    u_int segs_count = 0;
    while(segs_count * bks_per_seg  <= bks_tobe_read)
    {
        segs_count++;
    }

    return segs_count;
}

//input: log_addr 地址，返回长度为length的dis数据于buffer中
//-------Here now: length is always = bk_size (in bytes)----------
//-------from file layer-----------------------------------------
//----but this func is able to read > 1 bk_size data--------------
//--- if length > 1 bk_size -----------------------------------
//--------------------不读 Begin block----------------------------
//---------Note: flash memory I/O in unit of segment-------------
int Log_Read(LogAddress * log_addr, u_int length, void * buffer)
{
    //attemp to read from cache
    bool IS_IN_CACHE = read_cache(log_addr, length, buffer);   
    if(IS_IN_CACHE)
        return 0;

    //If not in cache, read data from disk, 
    //then store it on cache from the 1st cache seg
    //------------1. read data from flash-----------
    buffer = calloc(1, length);

    //choose the model of Flash
    Flash_Flags flags = FLASH_SILENT;
    
    u_int tmp = bks_per_seg * super_seg->seg_num;
    u_int * blocks = &tmp;
    Flash flash = Flash_Open(fl_file, flags, blocks); 
   
    u_int sec_offset = log_addr->seg_no * seg_size;
    
    u_int bks_remain = bks_per_seg - log_addr->bk_no;

    u_int bks_tobe_read = length_in_bk(length);
    
    u_int buffer_offset = 0;

    bool read_done = false;
   
    u_int segs_tobe_read = length_in_seg(bks_tobe_read); 

    u_int segs_read[segs_tobe_read];
    
    u_int i;
    for(i = 0; i < segs_tobe_read; i++)
    {
        segs_read[i] = log_addr->seg_no + i;
    }

    while(!read_done)
    {
        if(bks_tobe_read <= bks_remain)
        {
            //??need calloc before use buffer to this func? 
            void * new_buffer = calloc(1, seg_size * FLASH_SECTOR_SIZE);

            //Read the entire seg which contains the data
            //-------data may be > 1 seg size------------------
            Flash_Read(flash, sec_offset, seg_size, new_buffer);

            memcpy(buffer + buffer_offset, new_buffer
                    + log_addr->bk_no * bk_size * FLASH_SECTOR_SIZE, length);

            read_done = true;
        }
        else
        {
            //??need calloc before use buffer to this func? 
            void * new_buffer = calloc(1, seg_size * FLASH_SECTOR_SIZE);

            Flash_Read(flash, sec_offset, seg_size, new_buffer);
            
            memcpy(buffer + buffer_offset, new_buffer
                    + log_addr->bk_no * bk_size * FLASH_SECTOR_SIZE, 
                    bks_remain * bk_size * FLASH_SECTOR_SIZE);
            
            sec_offset += seg_size;

            buffer_offset += bks_remain * bk_size * FLASH_SECTOR_SIZE;
            length = length - bks_remain * bk_size * FLASH_SECTOR_SIZE;
            bks_remain = bks_per_seg;
            bks_tobe_read = length_in_bk(length);
            log_addr->seg_no++;
            log_addr->bk_no = 1;
        }
 
    }

    Flash_Close(flash);


    //----------2. store on cache-----------------
    //-------------2.1 check whether all are just updated----
    //---if all yes, set each's IS_JUST_UPDATE = false;
    for(i = 0; i < segs_tobe_read; i++)
    {
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
            if(!c_walker->IS_JUST_UPDATE)
            {
                //??need calloc before use buffer to this func? 
                void * new_buffer = calloc(1, seg_size * FLASH_SECTOR_SIZE);
                //choose the model of Flash
                flags = FLASH_SILENT;
                tmp = bks_per_seg * super_seg->seg_num;
                blocks = &tmp;
                flash = Flash_Open(fl_file, flags, blocks); 
                Flash_Read(flash, segs_read[i] * seg_size, seg_size, new_buffer);
                Flash_Close(flash);
                free(c_walker->seg);
                c_walker->seg = (Seg *)new_buffer;

                c_walker->IS_JUST_UPDATE = true;
                break;
            }
        }

    }



    return 0;

}

//----------grab one seg from flash memory into disk-------
//---------- to be written data---------------------------
void get_log_to_memory(LogAddress * log_addr)
{
    void * buffer = calloc(1, seg_size * FLASH_SECTOR_SIZE);
    //choose the model of Flash
    Flash_Flags flags = FLASH_SILENT;
    u_int tmp = bks_per_seg * super_seg->seg_num;
    u_int * blocks = &tmp;
    Flash flash = Flash_Open(fl_file, flags, blocks); 
    Flash_Read(flash, log_addr->seg_no * seg_size, seg_size, buffer);
    Flash_Close(flash);

    free(seg_in_memory);
    seg_in_memory = (Seg *)buffer; 

}


//-------------assist func--------------------------------------


//-----------set & grab a new log seg if necessary-----------
//--------call this func after write data to log----------------
void setLogTail()
{
    //If come to end of certain log seg
    if(tail_log_addr->bk_no == bks_per_seg -1)
    {
        //If come to every end of log structure
        if(tail_log_addr->seg_no == seg_num - 1)
        {
            tail_log_addr->seg_no = 1;
            tail_log_addr->bk_no = 1;
        }
        //else turn to next log seg's 1th bk
        else
        {
            tail_log_addr->seg_no += 1;
            tail_log_addr->bk_no = 1;
        }

        get_log_to_memory(tail_log_addr);
    }
    //just add 1 of bk_no
    else
    {
        tail_log_addr->bk_no += 1;
    }
}




//---------once tail_log_addr reaches certain log seg's end--------
//-------- push that log seg data into disk------------------------
//---------call this func before setLogTail()-------------------
void pushToDisk(LogAddress * log_addr)
{
    //If reaches certain log seg's end, write entire log seg to disk
    if(log_addr->bk_no == bks_per_seg -1)
    {
        //1.write to disk
        //choose the model of Flash
        Flash_Flags flags = FLASH_SILENT;

        //blocks : # of blocks in the flash
        u_int tmp = bks_per_seg * seg_num;
        u_int * blocks = &tmp;
        Flash   flash = Flash_Open(fl_file, flags, blocks);
        Flash_Write(flash, tail_log_addr->seg_no * seg_size, 
                seg_size, seg_in_memory);
        Flash_Close(flash);
    }
}

//-------------------write data log's one block---------------
//------ now   always   write 1 block size in bytes---------------
//-------each time call the log_write func----------------
void writeToLog(u_int inum, u_int block, void * buffer, LogAddress * log_addr)
{
    //---change the log_addr's content------------
    memcpy(seg_in_memory + bk_size * log_addr->bk_no * FLASH_SECTOR_SIZE,
            buffer, bk_size * FLASH_SECTOR_SIZE);

    Seg_sum_entry * sse_walker = seg_in_memory->begin_bk->ssum_bk->seg_sum_entry;
    //----change the seg_in_memory's Begin_bk---------
    while(sse_walker != NULL)
    {
        if(log_addr->bk_no == sse_walker->bk_no)
        {
            sse_walker->file_no = inum;
            sse_walker->file_bk_no = block;
            
            break;
        }
        sse_walker = sse_walker->next; 
    }

    //-------update seg usage table------------------
    u_int i;
    Seg_usage_table * sut_walker = super_seg->seg_usage_table;
    for(i = 0; i < seg_num; i++)
    {
        if(sut_walker->seg_no == log_addr->seg_no)
        {
            sut_walker->num_live_bk += 1;
            
//?? modify_time 怎么搞
            break;
        }
    }
}

//------------------------------------------------------------------




//-----------------------------------------------------------------
//将文件的inum(inode)的第block号块写入log, 写入log的地址为tail_log_addr,写入内i容
//----------------------input--------------------------------------
//--------input: length - always = fl_bk_size;
//-----------!!! 一次只写一个 block的数据--------------------------
//--------input: block - bk no within the file
int Log_Write(u_int inum, u_int block, u_int length, 
        void * buffer, LogAddress * log_addr)
{
    writeToLog(inum, block, buffer, log_addr);
    pushToDisk(log_addr);
    setLogTail();

    return 0;
}


//-------- erase blocks-----------------------------
//--------- in unit of  16  sectors-------------------------
//-------input: log_addr can only at 16 whole number location------
int Log_Free(LogAddress * log_addr, u_int length)
{

    u_int erase_bks = 0;
    while(erase_bks * FLASH_SECTORS_PER_BLOCK * FLASH_SECTOR_SIZE <= length)
        erase_bks++;


    u_int offset = (log_addr->seg_no * seg_size + log_addr->bk_no * bk_size)
        / FLASH_SECTORS_PER_BLOCK;



    //choose the model of Flash
    Flash_Flags flags = FLASH_SILENT;

    //blocks : # of blocks in the flash
    u_int tmp = bks_per_seg * seg_num;
    u_int * blocks = &tmp;
    Flash   flash = Flash_Open(fl_file, flags, blocks);
    Flash_Erase(flash, offset, erase_bks);
    Flash_Close(flash);

    return 0;
}

/*
int Log_Init(char * filename, Inode * ifile, u_int cachesize)
{
    //---------calloc memory------------------
//    filename = (char *)calloc(1, 8);
    
    Inode* tmp_file = (Inode *)calloc(1, sizeof(Inode));


    Flash_Flags flags = FLASH_SILENT;
    //blocks : # of blocks in the flash
    u_int tmp = sec_num / 16;
    u_int * blocks = &tmp;
    Flash   flash = Flash_Open(filename, flags, blocks);
    Flash_Read(flash, 0, seg_size);
//-------------------------------------------
    Super_seg * tmp_ss = ()
    
    memcpy(tmp_file, super_seg->checkpoint->ifile, sizeof(Inode));
    ifile = tmp_file;

    return 0;
}
*/
