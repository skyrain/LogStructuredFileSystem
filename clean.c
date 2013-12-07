//--- global variable in log.h ---------
//--- 初始化于LFS.c, 每pushToDisk中的if true，则 - 1---
//--- cleanning mechanism 
#define MIN_AVAILABLE_SEG_NUM 5
//--- CLEAN_SEG_NUM: clean this # of segs for future use----
//--- note: need to check that the entire clean seg should --
//-------------------- < seg_num ----------------------------
#define CLEAN_SEG_NUM 20
int available_seg_num;

//--- cleaning func in pushToDisk() ----
void clean_seg()
{
    //--- check whether start cleaning mechanism----
    if(available_seg_num < MIN_AVAILABLE_SEG_NUM)
    {
        //--- 
    }
}
