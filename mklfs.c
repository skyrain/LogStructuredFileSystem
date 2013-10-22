#include <stdio.h>
#include <stdlib.h>
#include "flash.h"
#include "log.h"

//??----------还没有管 wearlimit-------------------------

int main(int argc, char *argv[])
{
    //erase bk size
    //----------------------还未用---------------
    u_int er_bk_size = 16;   

//用户输入--------------------------------------------    
    //？？之后会作为参数设置,此处为默认值
    //用户输入
    wearlimit = 1000;
    fl_file = (char *)calloc(1, 8);
    strcpy(fl_file, "File");          //flash memory name

    //-----should always be 16 的整数倍
    sec_num = 1024;

    bk_size = 2;
    bks_per_seg = 32;
    seg_size = bks_per_seg * bk_size;
    
    // 1024个sector, 32个block组成一个seg
    //??注意应写程序保证若用户输入导致计算出的fl_seg_num非整数则让用户重新输入
    seg_num = sec_num / bk_size / bks_per_seg;
        
    //根据用户输入定义log的相应结构大小
   
    bk_content_size = bk_size * FLASH_SECTOR_SIZE;
    
    //cache seg 数量，此处为默认值
    cache_seg_num = 4; 
 //用户输入--------------------------------------------------


//-------------------------------------------------------------
    //create and format flash memory & create log in memory
    Log_Create();

    //create cache: store in dis_cache 
    create_cache();

}
