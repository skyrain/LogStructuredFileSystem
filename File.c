/*
 * File.c
 * Author: Weng
 * Created on: oct 11, 2013
 * This is the file layer c code
 */

#include <string.h>
#include "file.h"
#include "log.h"

#define FREE_BLOCK_NUM -1
#define FREE_

int File_Init(inode *Ino, int type)
{	
	//Init file time and filesize
	time_t time;     
        time(&time);     
        Ino->filesize = 0;        
        Ino->accese_Time = time;  
        Ino->change_Time = time;  
        Ino->create_Time = time;  
        Ino->modify_Time = time;  
				        
	//Init all direct blocks and indirect blocks.
        for(int i = 0; i<DIRECT_BK_NUM; i++)
        {
		Ino->direct_bk[i].seg_no = FREE_BLOCK_NUM;
		Ino->direct_bk[i].block_no = FREE_BLOCK_NUM;
	}
	
	Ino->indirect_bk.seg_no = FREE_BLOCK_NUM;
	Ino->indirect_bk.block_no = FREE_BLOCK_NUM;

 	return 0;

}

int File_Read(inode *Ino, int offset, int length, void *buffer){}

int File_Write(inode *Ino, int offset, int length, void *buffer){}

int File_Free(inode *Ino){}
