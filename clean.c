//--- global variable in log.h ---------
//--- 初始化于LFS.c, 每pushToDisk中的if true，则 - 1---
//--- cleanning mechanism 
#define MIN_AVAILABLE_SEG_NUM 6
//--- CLEAN_SEG_NUM: clean this # of segs for future use----
//--- note: need to check that the entire clean seg should --
//-------------------- < seg_num ----------------------------
#define CLEAN_SEG_NUM 20
//--- intialize in LFS.c to be == seg_num ------------------
//--- decrease in pushToDisk() -----------------------------
int available_seg_num;




//------- put funcs in dir.c -------------------
//----------------------------------------------
//-----------------------------=-----------------



//-------prerequisite: know the log_addr->seg_no has good bk to be used-----
//--- find available bk includes log_addr->bk itself-----------------------
void locate_log_addr_bk(LogAddress * log_addr, LogAddress * c_log_addr)
{
    LogAddress * tmp_addr = (LogAddress *)calloc(1, sizeof(LogAddress));
    int i = log_addr->bk_no;
    for(; i < bks_per_seg; i++)
    {
        tmp_addr->seg_no = log_addr->seg_no;
        tmp_addr->bk_no = i;
        if(is_in_wearlimit(tmp_addr) && !is_bk_in_use(tmp_addr))
        {
            c_log_addr->seg_no = log_addr->seg_no;
            c_log_addr->bk_no = i;
            return;
        }
    }
    free(tmp_addr);
}

//--------locate tail_log_addr from first normal segment----------------
//---- input: log_addr: addr that need to be changed to a new place ----
void locate_log_addr_from_begin(LogAddress * log_addr)
{
    int i;
    for(i = 1; i < seg_num; i++)
    {
        //--- check seg_usage_table's is_checkpoint ----
        Seg_usage_table * sut_walker = checkpoint->seg_usage_table;
        int j = i;
        while(j > 1)
        {
            sut_walker = sut_walker->next;
            j--;
        }

        //---- if the seg is not seg storing cp---------
        //---- & the seg is not seg_in_memory ---------
        if(sut_walker->is_checkpoint == false)
        {
            LogAddress * tmp_addr = (LogAddress *)calloc(1, sizeof(LogAddress));
            tmp_addr->seg_no = i;
            tmp_addr->bk_no = 1;
            //---- if this seg has available bk to store data---
            if(!is_remain_seg_not_usable(tmp_addr))
            {
                locate_tail_log_addr_bk(tmp_addr, log_addr);
                free(tmp_addr);
                break;
            } 
            free(tmp_addr);
        }
    }
}

//--------  find the next bk < wearlimit and available --------
//---- in    other    seg: (not cp seg, not seg_in_memory) ----
//----- input: live bk's log addr -------------------------
void find_new_addr_for_live_bk(LogAddress * log_addr)
{
    LogAddress * tmp_addr = (LogAddress *)calloc(1, sizeof(LogAddress));

    //If come to every end of log structure
    if(log_addr->seg_no == seg_num - 1)
    {
        locate_log_addr_from_begin(log_addr);
    }
    //else turn to check next log seg's 1th bk
    else
    {
        bool can_find_in_back = false;
        tmp_addr->seg_no = log_addr->seg_no + 1;
        tmp_addr->bk_no = 1;
        //--- check seg_usage_table's is_checkpoint ----
        Seg_usage_table * sut_walker = checkpoint->seg_usage_table;
        int j = tmp_addr->seg_no;
        while(j > 1)
        {
            sut_walker = sut_walker->next;
            j--;
        }
        while(tmp_addr->seg_no < seg_num)
        {
            //--- note: add condition that the checked seg should not be--
            //--- seg storing checkpoint---------------------------------
            if(!is_remain_seg_not_usable(tmp_addr) 
                    && sut_walker->is_checkpoint == false)
            {
                locate_log_addr_bk(tmp_addr, log_addr);
                can_find_in_back = true;
                break;
            }
            tmp_addr->seg_no++;
        }

        if(!can_find_in_back)
            locate_log_addr_from_begin(log_addr);
    }
}



void move_live_bk(LogAddress * log_addr)
{
    //--- 1. read log_addr bk out of original place ---
    void * buffer = calloc(1, seg_size * FLASH_SECTOR_SIZE);
    Flash_Flags flags = FLASH_SILENT;
    u_int tmp = sec_num / FLASH_SECTORS_PER_BLOCK;
    u_int * blocks = &tmp;
    Flash flash = Flash_Open(fl_file, flags, blocks);
    Flash_Read(flash, log_addr->seg_no * seg_size, seg_size, buffer);

    void * buffer = calloc(1, seg_size * FLASH_SECTOR_SIZE);
    copy_log_to_memory(log_addr->seg_no, buffer);
    Seg * tseg = (Seg *)buffer;
    Seg_sum_entry * sse_walker = tseg->begin_bk->ssum_bk->seg_sum_entry;
    int file_no = 0;
    int file_bk_no = 0;
    while(sse_walker != NULL)
    {
        if(sse_walker->bk_no == log_addr->bk_no)
        {
            file_no = sse_walker->file_no;
            file_bk_no = sse_walker->file_bk_no;
            break;
        }
        sse_walker = sse_walker->next;
    }
    //--- 2. find new location -----------------------
    find_new_addr_for_live_bk(log_addr); 
    //--- 3. write bk to new location -----------------
    Flash_Write(flash, log_addr->seg_no * seg_size, seg_size, buffer);
    //--- 4. change the new location's seg's seg_sum_entry---
    void * tbuffer = calloc(1, seg_size * FLASH_SECTOR_SIZE);
    copy_log_to_memory(log_addr->seg_no, tbuffer);
    Seg * ttseg = (Seg *)tbuffer;
    Seg_sum_entry * sse_walker = ttseg->begin_bk->ssum_bk->seg_sum_entry;
    while(sse_walker != NULL)
    {
        if(sse_walker->bk_no == log_addr->bk_no)
        {
            sse_walker->file_no = file_no;
            sse_walker->file_bk_no = file_bk_no;
            break;
        }
        sse_walker = sse_walker->next;
    }

    //--- 5. change the Inode Block pointer points to this new location--

    /////////////////////////




    //-----? 等翁旭东---------------    

}

//-------- erase seg for cleaning mechanism--------------------
//--------- in unit of  16  sectors---------------------------
//-------- 调用改函数前(seg_no)th seg的num_live_bks & --------
//---- seg_sum_entry 已经updated ------------------------------
int Log_Free(int seg_no)
{
    void * buffer = calloc(1, seg_size * FLASH_SECTOR_SIZE);
    copy_log_to_memory(seg_no, buffer);
    Seg * seg = (Seg *)buffer;

    //----1. move live bks to other segs -----------------
    int i;
    for(i = 1; i < bks_per_seg; i++)
    {
        int j = i;
        Seg_sum_entry * sse_walker = seg->begin_bk->ssum_bk->seg_sum_entry; 
        while(j > 1)
        {
            sse_walker = sse_walker->next;
            j--;
        }
        //--- check ith bk's seg_sum_entry -------------
        //-- if points to a live bk --------------------
        if(sse_walker->file_no != -1)
        {
            LogAddress * tmp_addr = (LogAddress *)calloc(1, sizeof(LogAddress));            tmp_addr->seg_no = seg_no;
            tmp_addr->bk_no = i;
            move_live_bk(tmp_addr);
            free(tmp_addr);           
        }

    }    
    //----2. free this seg ------------------------------
    //choose the model of Flash
    Flash_Flags flags = FLASH_SILENT;
    //blocks : # of blocks in the flash
    u_int tmp = sec_num / FLASH_SECTORS_PER_BLOCK;
    u_int * blocks = &tmp;
    Flash   flash = Flash_Open(fl_file, flags, blocks);
    u_int offset = (seg_size / FLASH_SECTORS_PER_BLOCK) * seg_no; 
    u_int erase_bks = seg_size / FLASH_SECTORS_PER_BLOCK;
    Flash_Erase(flash, offset, erase_bks);
    //--- reconstruct the seg structure -----------------
    void * n_seg_buffer = calloc(1, seg_size * FLASH_SECTOR_SIZE);
    bytes_offset = 0;
    Seg * ts = (Seg *)n_seg_buffer;
    ts->begin_bk = n_seg_buffer + sizeof(Seg);
    bytes_offset += sizeof(Seg);
    //--------begin bk----------------------------------
    Begin_bk * bb = (Begin_bk *)(n_seg_buffer + bytes_offset);
    bb->seg_no = i;
    bb->ssum_bk = n_seg_buffer + bytes_offset + sizeof(Begin_bk);
    bytes_offset += sizeof(Begin_bk);
    //----seg sum bk of begin bk----------------------------
    Seg_sum_bk * ssb = (Seg_sum_bk *)(n_seg_buffer + bytes_offset);
    ssb->bk_no = 0;
    ssb->seg_sum_entry = n_seg_buffer + bytes_offset + sizeof(Seg_sum_bk);
    bytes_offset += sizeof(Seg_sum_bk);
    //---- seg sum entry of seg sum bk---------------------- 
    int j;
    for(j = 1; j < bks_per_seg; j++)
    {
        Seg_sum_entry * sse = (Seg_sum_entry *)(n_seg_buffer
                + bytes_offset);
        sse->bk_no = j;
        sse->file_no = -1;
        sse->file_bk_no = -1;
        bytes_offset += sizeof(Seg_sum_entry);
        if(i != bks_per_seg - 1)
            sse->next = n_seg_buffer + bytes_offset;
        else
            sse->next = NULL;
    }

    Flash_Write(flash, seg_size * seg_no, seg_size, n_seg_buffer);
    free(n_seg_buffer);

    Flash_Close(flash);

    return 0;
}

//--- cleaning func in pushToDisk() ---------------
//-- note: checkpoint 不参与cleaning mechanism ----
void clean_seg()
{
    //--- check whether start cleaning mechanism----
    if(available_seg_num < MIN_AVAILABLE_SEG_NUM)
    {
        int i;

        //--- update seg_usage_table's num_live_bk attribute-----------
        //--- & update each seg's seg_sum_entry -----------------------
        for(i = 1; i < seg_num; i++)
        {
            //---scan segs which store data(not checkpoint seg),--------
            //--- check seg_usage_table's is_checkpoint ----
            Seg_usage_table * sut_walker = checkpoint->seg_usage_table;
            int j = i;
            while(j > 1)
            {
                sut_walker = sut_walker->next;
                j--;
            }
            
            //---update num_live_bk attribute in seg_usage_table--
            if(sut_walker->is_checkpoint == false)
            {
                void * tbuffer = calloc(1, seg_size * FLASH_SECTOR_SIZE);
                copy_log_to_memory(i, tbuffer);
                Seg * tseg = (Seg *)tbuffer;

                // Initial the temp parameter for segment summery entry
                int status = 0;
                int NumLiveBlock = 0;
                //ifile = (Inode *)calloc(1, inode_ifile->filesize);
                //status = File_Read(inode_ifile, 0, inode_ifile->filesize, ifile);

                Seg_sum_entry * SegSumEntry = tseg->begin_bk->Seg_sum_bk->Seg_sum_entry;

                int inum;
                int file_bk; 
                int i;

                for(i = 0; i < bks_per_seg; i++){
                    inum = SegSumEntry->file_no; 
                    file_bk = SegSumEntry->file_bk_no;

                    if(SegSumEntry->bk_no == ifile[inum].direct_bk[file_bk].bk_no)
                    {
                        NumLiveBlock ++;
                    }
                    else
                    {
                        SegSumEntry->bk_no = FREE_BLOCK_NUM;
                        SegSumEntry->file_no = FREE_BLOCK_NUM;
                    }

                    // link to the next block in this segment
                    SegSumEntry = SegSumEntry->next;
                }

                // update the number of live blocks
                sut_walker->num_live_bk = num_live_bk;
           }
        }

        //---- do cleaning--------------------------
        //--- determine how many segs to clean------
        int clean_num = available_seg_num + CLEAN_SEG_NUM;
        if(clean_num > seg_num)
            clean_num = seg_num;
        //--according to num_live_bk get to know maximum clean_num further--
        int total_num_live_bk = 0;
        int data_seg_num = 0;
        Seg_usage_table * sut_walker = checkpoint->seg_usage_table;
        while(sut_walker != NULL)
        {
            if(sut_walker->is_checkpoint == false)
            {
                total_num_live_bk += sut_walker->num_live_bk;
                data_seg_num ++;
            }
            
            sut_walker = sut_walker->next;
        }
        
        int free_bk_num = data_seg_num * (bks_per_seg - 1) - total_num_live_bk;
        int can_clean_seg_num = free_bk_num / (bks_per_seg - 1); 
        if(can_clean_seg_num < 1)
        {
            printf("Cannot clean anymore! \n");
        }
        else
        {
            printf("Start cleaning!\n");
            //-- set real world can be cleaned seg num ----------
            if(can_clean_seg_num < clean_num)
                clean_num = can_clean_seg_num;

            //---- do clean ---------------------------------
            for(i = 0; i < clean_num; i++)
            {
                Log_Free(i);
            }  
        }

    }
}
