/*
 * File.c
 * Created on: oct 11, 2013
 * This is the file layer c code
 */

#include <string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "File.h"
//#include "log.h"
#include <errno.h>
#include "clean.c"

//--------?? log.h already has this-------
#define DIRECT_BK_NUM 4

#define FREE_BLOCK_NUM -1
#define READ_ERROR -2
#define INDIRECT_BK_NUM -3
// temp
//#define 

//u_int BLOCK_SIZE;
//extern Log_Read(u_int disk_seg, u_int disk_bk_no, u_int length, void *buffer); 

int File_Init(Inode *Ino, int type)
{	
	//Init file time and filesize
	time_t t;     
        time(&t);     
        Ino->filesize = 0;        
	
	int i;	
	//Init all direct blocks and indirect blocks.
        for(i = 0; i<DIRECT_BK_NUM; i++)
        {
		Ino->direct_bk[i].seg_no = FREE_BLOCK_NUM;
		Ino->direct_bk[i].bk_no = FREE_BLOCK_NUM;
	}

        Ino->access_Time = t;  
        Ino->change_Time = t;  
        Ino->create_Time = t;  
        Ino->modify_Time = t;  
	//Ino->mode = 0;	

	//some problem with the indirect and direct block structure
	Ino->indirect_bk.seg_no = FREE_BLOCK_NUM;
	Ino->indirect_bk.bk_no = FREE_BLOCK_NUM;

 	return 0;

}

int File_Read(Inode *Ino, int offset, int length, void *buffer)
{
	// Ino is the file to read from
	// offset : where to start reading in bytes
	// length : in bytes
	// buffer is where to fill the data in
	int status = 0;

	if(length == 0)
		return status;
	if(Ino->filesize < offset)
		return status;

	// where the file end in block.
	u_int BlockSize_byte = BLOCK_SIZE;
	int FileEndBlock = Ino->filesize / BlockSize_byte;
	if(Ino->filesize % BlockSize_byte ==0 && Ino->filesize != 0)
		FileEndBlock--;

	// if the read request beyond the one block size then keep synchronize.
	int ReadEndBlock;
	if(Ino->filesize < offset + length)
	{
		ReadEndBlock = FileEndBlock;
		length = Ino->filesize - offset;
	}
	else
	{
		ReadEndBlock = (offset + length) / BlockSize_byte; // where the read end in block
		if((offset + length) % BlockSize_byte == 0) // if just at the end of a block
			ReadEndBlock--;
	}
	printf("ReadEndBlock: %i\n", ReadEndBlock);

	// how many blocks a the read request need and where to start in a block
	int ReadStartBlock = offset / BlockSize_byte;
	int InBlockStart = offset % BlockSize_byte;
	int NumBlocks = ReadEndBlock - ReadStartBlock + 1;
	printf("NumBlocks: %i\n", NumBlocks);

	// a read request cannot over the max blocks number a Inode has
	int MaxBlocks = DIRECT_BK_NUM + BlockSize_byte/sizeof(Block_pointer);
	if(ReadEndBlock >= MaxBlocks)
		return READ_ERROR;
	
	// init temp buffer
	void *ReadBuffer = calloc(NumBlocks, BlockSize_byte); 
	void *ReadPointer = ReadBuffer;

	// read from log, set the disk addr to read from.
	Block_pointer blockPointer;
	LogAddress LogAddr;
	int ReadBlockNumber;
	for(ReadBlockNumber = ReadStartBlock; ReadBlockNumber <= ReadEndBlock; ReadBlockNumber++)
	{
		//get pointer to the first block;
		//read block by block to the ReadBuffer;
		//get one of the four direct block from the Inode. and cpy to the Block_pointer
		Get_Block_pointer(Ino, ReadBlockNumber, &blockPointer);
		LogAddr.seg_no = blockPointer.seg_no;
		LogAddr.bk_no = blockPointer.bk_no;
		if(blockPointer.seg_no != FREE_BLOCK_NUM && blockPointer.bk_no != FREE_BLOCK_NUM)
		{
			status = Log_Read(&LogAddr, BlockSize_byte, ReadPointer);// if read more than one segment, I cant read if the segment is the last segment in log
			if(status)
			{
				return status;
				printf("Log_Read fail\n");
			}

			printf("Reading the file %i: \n", Ino->ino);
		}
		else
		{
			printf("Cannot read %i because the block %i is not Init \n", Ino->ino, ReadBlockNumber);
		}
		// append the content to the init buffer
		ReadPointer += BlockSize_byte;
	}
	// fill in the final buffer which contain the result
	// , update inode and free the temp buffer.
	memcpy(buffer, ReadBuffer+InBlockStart,length);
	time(&Ino->access_Time);	
	free(ReadBuffer);
	
	return status;
}


int File_Write(Inode *Ino, int offset, int length, void *buffer)
{
	//----- check wheher do cleaning mechanism ----
	clean_seg();	
	// length to be written in bytes
	
	int status = 0;
	u_int BlockSize_byte = BLOCK_SIZE;

	// next define where start to write, if offset out of range
	// write at the last block of the file
	int fileEndBlock = Ino->filesize / BlockSize_byte;
	if((Ino->filesize % BlockSize_byte == 0 )&& fileEndBlock != 0)
		fileEndBlock --;

	int writeStartBlock;
	if(Ino->filesize < offset)
		writeStartBlock = fileEndBlock;
	else
		writeStartBlock = offset / BlockSize_byte;

	// next, define which block end the write and from which byte
	// start to write at the writeStartBlock 
	int innerBlockStartOffset = offset % BlockSize_byte;
	int writeEndBlock = (offset + length) / BlockSize_byte;

	if((offset + length) % BlockSize_byte == 0)
		writeEndBlock--;

	// deal with some write error such as out of range.
	// numBlocks is how many blocks going to write
	int numBlocks = writeEndBlock - writeStartBlock + 1;
	int maxNumBlocks = DIRECT_BK_NUM + BlockSize_byte/sizeof(Block_pointer);
	if(writeEndBlock >= maxNumBlocks)
		return -EFBIG;

	void *writeBuffer = calloc(numBlocks, BlockSize_byte);
	
	// start to copy part of blocks of data into the temp buffer, at first we need to 
	// read some orginal data from the disk. start at writeStartBlock to readEndBlock
	LogAddress firstBlockAddr;
	LogAddress lastBlockAddr;
	Block_pointer blockPointer;
	if( fileEndBlock >= writeStartBlock)
	{
		int readEndBlock;
		if(fileEndBlock > writeEndBlock)
			readEndBlock = writeEndBlock;
		else
			readEndBlock = fileEndBlock;
		
		//set a pointer to the first block to write
		//Block_pointer blockPointer;
		Get_Block_pointer(Ino, writeStartBlock, &blockPointer);
		firstBlockAddr.seg_no = blockPointer.seg_no;
		firstBlockAddr.bk_no = blockPointer.bk_no;
		// read the block to write buffer
		if(blockPointer.seg_no != FREE_BLOCK_NUM && blockPointer.bk_no != FREE_BLOCK_NUM)
		{
			status = Log_Read(&firstBlockAddr, BlockSize_byte, writeBuffer);
		}
		if(status)
		{
			printf("error, fail to read the first block the file in log\n");
			return status;
		}

		if(numBlocks > 1)
		{
			// set a pointer to the last block
			Get_Block_pointer(Ino, readEndBlock, &blockPointer);
			lastBlockAddr.seg_no = blockPointer.seg_no;
			lastBlockAddr.bk_no = blockPointer.bk_no;
			if(blockPointer.seg_no != FREE_BLOCK_NUM && blockPointer.bk_no != FREE_BLOCK_NUM)
			{	
				status = Log_Read(&lastBlockAddr, BlockSize_byte, writeBuffer + (numBlocks-1)*BlockSize_byte);
			}			
			if(status)
			{
				printf("error, fail to read the file's last block in log \n");
				return status;
			}
		}
	}
	
	// overwrite the file,  write start at innerBlockStartOffset, with length from the buffer
	memcpy(writeBuffer + innerBlockStartOffset, buffer, length);

	void *writePointer = calloc(1, BlockSize_byte);

	// Log_Write of block one by one and buffer one by one.
	int writeBlock;
	LogAddress AddrWrite;
	LogAddress *tailaddr;
	Block_pointer *blockBuffer = NULL;
	for(writeBlock = 0; writeBlock < numBlocks; writeBlock ++ )
	{ 	
		// write the orginal file which file block number is AddrWrite.bk_no
		// to the tail of the segment
		//AddrWrite.seg_no = blockPointer.seg_no;
		//AddrWrite.bk_no = blockPointer.bk_no;
		
		// File block number to be write, pass to the log write.
		int fileBlockNumber = writeStartBlock + writeBlock;
		if( fileBlockNumber < DIRECT_BK_NUM )
		{
			//update seg usage table if the block is unused before
			if(Ino->direct_bk[fileBlockNumber].seg_no != FREE_BLOCK_NUM)
				Decrement_Seg_Usage(Ino->direct_bk[fileBlockNumber].seg_no,
						Ino->direct_bk[fileBlockNumber].bk_no);
		}
		else // if there is indirect block
		{
			if(blockBuffer == NULL)
			// if we haven't read indirect block, read them off the flash
			{
				blockBuffer = (Block_pointer *)calloc(1, BlockSize_byte);
				if(Ino->indirect_bk.seg_no == FREE_BLOCK_NUM)
				{
					int j;
					for( j = 0; j < BlockSize_byte/sizeof(Block_pointer); j++)
					{
						blockBuffer[j].seg_no = FREE_BLOCK_NUM;
						blockBuffer[j].bk_no = FREE_BLOCK_NUM;
					}
				}
				else // read the file indirect block 
				{       LogAddress *temp_logaddress;
					temp_logaddress->seg_no = Ino->indirect_bk.seg_no;
					temp_logaddress->bk_no = Ino->indirect_bk.bk_no;
					status = Log_Read(temp_logaddress,1,blockBuffer);
					if(status)
					{
						return status;
					}
				}
			}
			// change this pointer in indirect block
			if(blockBuffer[fileBlockNumber - DIRECT_BK_NUM].seg_no != FREE_BLOCK_NUM)
				Decrement_Seg_Usage(blockBuffer[fileBlockNumber - DIRECT_BK_NUM].seg_no, blockBuffer[fileBlockNumber - DIRECT_BK_NUM].bk_no);
		}
	}
	
	// write the indirect block content into the buffer

		//Get_Block_pointer(Ino, writeBlock, &blockPointer);
	
	// ?? log_write 判断每次写是否在同一个block.
	for( writeBlock = 0; writeBlock < numBlocks; writeBlock ++)
	{
		int fileBlockNumber = writeStartBlock + writeBlock;
		tailaddr = tail_log_addr;

		// update the file blocks that are modified.
		if (fileBlockNumber < DIRECT_BK_NUM)
		{
			Ino->direct_bk[fileBlockNumber].seg_no = tailaddr->seg_no;
			Ino->direct_bk[fileBlockNumber].bk_no = tailaddr->bk_no;
		}
		else // this is for indirect blocks
		{
			blockBuffer[fileBlockNumber - DIRECT_BK_NUM].seg_no = tailaddr->seg_no;
			blockBuffer[fileBlockNumber - DIRECT_BK_NUM].bk_no = tailaddr->bk_no;
		}

		// write to the flash
		memcpy(writePointer, writeBuffer + BlockSize_byte*writeBlock, BlockSize_byte);
		status  = Log_Write(Ino->ino, fileBlockNumber, BlockSize_byte, writePointer, tailaddr);
		if(status)
		{
			printf("fail to write on the log, Log_Write\n");
			return status;
		}
		
		/*
		if( fileBlockNumber < DIRECT_BK_NUM)
		{
			Ino->direct_bk[fileBlockNumber].seg_no = 
			Ino->direct
		}
		*/
	}
	// if w read write to indirect block need to clean up
	if (blockBuffer != NULL)
	{
		if(Ino->indirect_bk.seg_no != FREE_BLOCK_NUM)
			Decrement_Seg_Usage(Ino->indirect_bk.seg_no, Ino->indirect_bk.bk_no);
		// push it back to the log
		tailaddr = tail_log_addr;
		status = Log_Write( Ino->ino, INDIRECT_BK_NUM, BlockSize_byte, blockBuffer, tailaddr);
		if(status){ return status; }
		free(blockBuffer);
	}
	//update inode, File layer tought we  write something on the disk, so it should update
	// the disk locations into inode ---???---
	if(length + offset > Ino->filesize)
	{
		Ino->filesize = offset + length;
	}
	
	time_t t;
	time(&t);
	Ino->access_Time = t;
	Ino->change_Time = t;

	free(writeBuffer);
	free(writePointer);

	return status;

}
	
int File_Free(Inode *Ino)
{
	printf("Freeing the file %i\n", Ino->ino);

	int status = 0;

	status = File_Truncate(Ino, 0);
	if(status)
	{
		printf("ERROR: when dropping the file");
		return status;
	}
	
	// ifile delete can be modified as linked list, to save the space but waste the time.
	memset(Ino, 0, sizeof(Inode));

	return 0;

}

/*
int File_Drop(Inode *Ino, int offset)
{
	Block_pointer *BlockBuffer = NULL;
	Block_pointer *BlockPointer;
	int BlockSize_byte = BLOCK_SIZE;
	int FileBlockNumber;
	int FirstBlockDrop;
	int PartialBlockLeft;

	int status = 0;

	if(offset > Ino->filesize)
	{
		printf("Drop beyond the range of file! \n");
		return status;
	}

	double BlockNum = offset / BlockSize_byte;
	FirstBlockDrop = ceil(BlockNum);
	PartialBlockLeft = floor(BlockNum);


	//decide which file are going to drop.
	if(PartialBlockLeft * BlockSize_byte != offset)
	{

	}
}
*/

// change the size of file
int File_Truncate(Inode *myNode, off_t offset)
{		
        u_int BlockSize_byte = BLOCK_SIZE;

	Block_pointer *blockBuffer = NULL;
        Block_pointer *blockPointer;
        int fileBlockNumber;
        int firstBlockToRemove, partialBlockRewrite;
        int status = 0;

        // Check to be sure the file is bigger than offset
        if (offset > myNode->filesize)
        {
            printf( "Truncate beyond end of file.\n");
            return -EPERM;
        }

        // Decide which block to start truncating
        float temp = (float) offset/ BLOCK_SIZE;
        firstBlockToRemove = ceil(temp);
        partialBlockRewrite = floor(temp);
        //partialBlockRewrite = firstBlockToRemove - 1;
		
		// wanna read or wirte not in the BLOCK_SIZE
        if(partialBlockRewrite * BLOCK_SIZE != offset){
                // there is a partial block truncated, handle it
                int sizePartBlock = offset - partialBlockRewrite * BLOCK_SIZE;
                void *buffer = calloc(1, BLOCK_SIZE);
				
                status = File_Read(myNode, partialBlockRewrite * BLOCK_SIZE,
                                                        sizePartBlock, buffer);
                if( status ){printf("read the truncating file wrong \n"); return status;}
                status = File_Write(myNode, partialBlockRewrite * BLOCK_SIZE, BLOCK_SIZE, buffer);
                if( status )
                {printf("Fail to truncate the partial block \n"); return status;}
        }

        if (firstBlockToRemove * BLOCK_SIZE > myNode->filesize){
                printf("No blocks to remove.\n");
        }else{ // release blocks at end of file
                int maxFileSizeBlocks = DIRECT_BK_NUM + BLOCK_SIZE/sizeof(Block_pointer);
                for( fileBlockNumber = firstBlockToRemove; fileBlockNumber < maxFileSizeBlocks ; fileBlockNumber++)
                {
                        if( fileBlockNumber < DIRECT_BK_NUM )
                        {
                                blockPointer = &myNode->direct_bk[fileBlockNumber];
                        }
			else
			{
				if(myNode->indirect_bk.seg_no == FREE_BLOCK_NUM)
					break;
				if(blockBuffer == NULL)
				// if we haven't read indirect block, read them
				{
					blockBuffer = (Block_pointer *)calloc(1, BLOCK_SIZE);
					LogAddress LogAdd;
					LogAdd.seg_no = myNode->indirect_bk.seg_no;
					LogAdd.bk_no = myNode->indirect_bk.bk_no;					
					status = Log_Read(&LogAdd, BLOCK_SIZE, blockBuffer);
					if(status) { return status; }
				}
				// change this pointer in indirect block
				blockPointer = &blockBuffer[fileBlockNumber - DIRECT_BK_NUM];	
			}

                        if( blockPointer->seg_no == FREE_BLOCK_NUM ){
                                // just skip over unused blocks - there may be used ones later
                        }else{
								// decrement the seg usage table
                                Decrement_Seg_Usage( blockPointer->seg_no, blockPointer->bk_no );
                                blockPointer->bk_no = FREE_BLOCK_NUM;
                                blockPointer->seg_no = FREE_BLOCK_NUM;
                        }
                }
        }
	
	if(blockBuffer != NULL)
	{
		if(myNode->indirect_bk.seg_no != FREE_BLOCK_NUM)
			Decrement_Seg_Usage(myNode->indirect_bk.seg_no, myNode->indirect_bk.bk_no);

		// if the indirect block is no logner needed, blank it out.
		if(offset < DIRECT_BK_NUM*BlockSize_byte)
		{
			myNode->indirect_bk.seg_no = FREE_BLOCK_NUM;
			myNode->indirect_bk.bk_no = FREE_BLOCK_NUM;
		}else
		{       
			// push the indirect block back to the log
			LogAddress *tailLogAddr = tail_log_addr;
			status = Log_Write(myNode->ino, INDIRECT_BK_NUM, BlockSize_byte, blockBuffer, tailLogAddr);
			if(status) { return status; }
		}
		free(blockBuffer);
	}
        myNode->filesize = offset;
	
        return status;

}

void File_Layer_Destroy()
{
    ;
//	Log_Destroy();
}

// Init the file layer, ready to read and write the file from the info 
// of ifile.
int File_Layer_Init(char *filename, Inode *ifile, u_int cachesize)
{
	// filename is the flash file that are going to open, these argument are passed by user, layer to layer.
	// then we need to read the ifile from the checkpoint to the
	// memory, please return like:*ifile = &(Log.checkPoint.ifile)
	// in phase 2, we need add more argument for lfs [option], 
	// such as cachesize and the interval of checkpoint time.
	return  Log_Init(filename, ifile, cachesize /* other arguement in phase 2*/);
}

//get one of the four direct block from the Inode. and cpy to the Block_pointer.
void Get_Block_pointer(Inode *Ino, int BlockNumber, Block_pointer *bp)
{
	// beyond the current file
	if((BlockNumber * BLOCK_SIZE) > Ino->filesize)
	{
		bp->seg_no = FREE_BLOCK_NUM;
		bp->bk_no = FREE_BLOCK_NUM;
	}

	if(BlockNumber < DIRECT_BK_NUM)
	{ 
		memcpy(bp, Ino->direct_bk + BlockNumber, sizeof(Block_pointer));
        //Block_pointer * test;
        
        // Block_pointer* tbp = (Block_pointer *)calloc(1, sizeof(Block_pointer));
        //tbp->seg_no = tail_log_addr->seg_no;
        //tbp->bk_no = tail_log_addr->bk_no;
        //memcpy(bp, tbp, sizeof(Block_pointer));
        //free(tbp);
   	}
	else
	{
		printf("BlockNumber out of DIRECT_BK_NUM\n");
		return;
	}
}


int Decrement_Seg_Usage( int segment, int block)
{
        // if the segment is at tail of the log, then set the corresponding block
        // to be unused
        /*
        if(segment == tail_log_addr->seg_no)
        {
                LogAddress *log_addr;
                log_addr->seg_no = segment;
                log_addr->bk_no = 0;

                void get_log_to_memory(LogAddress * log_addr);
                LogAddress *tail_temp = tail_log_addr;

                while() 
                {
                                
                }

                if( seg_sum_entry->bk_no != block)
                {
                        file_no = FREE_BLOCK_NUM;
                        file_bk_no = FREE_BLOCK_NUM;
                        bk_no = FREE_BLOCK_NUM;
                }
        }
        else
        {
                Checkpoint *CK = checkpoint;
                Seg_usage_table *entry = CK->seg_usage_table;
                while( entry->seg_no != segment)
                {
                        entry = entry->next;
                }
                
                if( entry->seg_no = segment)
                {
                        entry->num_live_bk -= 1;
                }
        }
        */
}


