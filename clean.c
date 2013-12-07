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

//--- cleaning func in pushToDisk() ---------------
//-- note: checkpoint 不参与cleaning mechanism ----
void clean_seg()
{
    //--- check whether start cleaning mechanism----
    if(available_seg_num < MIN_AVAILABLE_SEG_NUM)
    {
        int i;

        //--- update seg_usage_table's num_live_bk attribute-----------
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

                //--- update tseg's seg_usage_table according to---
                //-- tseg's seg_sum_entry & all the inode info-----

                //-------?? 等翁旭东填写update----------------
                //--1.注意考虑file被删除后，其inode是找不到的--
                //--所有seg的某个bk中，找不到相应file_no的删除之--
                //--2.重置过期的seg之seg_sum_entry的file_no, bk_no 为-1 ---


                //------?? end----------------------------
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
