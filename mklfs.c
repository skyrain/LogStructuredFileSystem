#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "flash.h"
#include "log.h"

//??----------还没有管 wearlimit-------------------------

int main(int argc, char * argv[])
{
    //erase bk size
    //----------------------还未用---------------
    u_int er_bk_size = 16;   

    
    //用户输入--------------------------------------------    
    wearlimit = 1000;
    fl_file = (char *)calloc(1, 8);
    strcpy(fl_file, "File");         

    //-----should always be 16 的整数倍
    sec_num = 1024;
    bk_size = 2;
    bks_per_seg = 32;

//??
    //------------file layer 设置 cache---------------    
    //cache seg 数量，此处为默认值
    cache_seg_num = 4; 
    //用户输入--------------------------------------------------

/*  
  mklfs [options] file
    -l size, --segment=size
        Segment size, in blocks. The default is 32.
    -s sectors, --sectors=sectors
        Size of the flash, in sectors.  The default is 1024.
    -b size, --block=size
        Size of a block, in sectors. The default is 2 (1KB).
    -w limit, --wearlimit=limit
        Wear limit for erase blocks. The default is 1000.

*/
    int ch;  
    while ((ch = getopt(argc, argv, "l:s:b:w:")) != -1)  
    {  
        switch (ch) {  
            case 'l': 
                if(atoi(optarg) >= 65536)
                {
                    printf("Too many blocks per segment!\n");
                    return 0;
                } 
                bks_per_seg = (u_int)atoi(optarg);
                break;  
            case 's':
                if(atoi(optarg) >= 65536)
                {
                    printf("Too big flash memory!\n");
                    return 0;
                }
                else
                { 
                    sec_num = (u_int)atoi(optarg);
                }
                break;  
            case 'b':
                bk_size = (u_int)atoi(optarg);
                break;  
            case 'w': 
                if(atoi(optarg) >= 65536)
                {
                    printf("Too big wearlimit!\n");
                }
                wearlimit = (u_int)atoi(optarg);
                break;  
            case '?':
                break;  
        }
    }  
    strcpy(fl_file, argv[argc - 1]);

 //??在多考虑其他边缘情况比如 sec_num = 0
    seg_size = bks_per_seg * bk_size;
    seg_num = sec_num / bk_size / bks_per_seg;
    bk_content_size = bk_size * FLASH_SECTOR_SIZE;

    if(sec_num % FLASH_SECTORS_PER_BLOCK != 0)
    {
        printf("Total sectors must be 16 whole times!\n");
        return 0;
    }

    if(seg_size % FLASH_SECTORS_PER_BLOCK != 0)
    {
        printf("Seg size must be 16 whole times!\n");
        return 0;
    }
    
    if(seg_size > sec_num)
    {
        printf("Segment size exceed the entire flash memory!\n");
        return 0;
    }
    
    if(sec_num % bk_size != 0)
    {
        printf("Not whole number of blocks!\n");
        return 0;
    }

    if((sec_num / bk_size) % bks_per_seg != 0)
    {
        printf("Not whole number of segs!\n");
    }


//-------------------------------------------------------------
    //create and format flash memory & create log in memory
    Log_Create();

    //create cache: store in dis_cache 
    create_cache();

}
