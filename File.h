/*
 * File.h
 * Created on: Oct 11, 2013
 * This is the file layer header
 */

//#include <sys/types.h>
#include "log.h"
#include <sys/types.h>

/*
 * inode is the inode of the file to be accessed, 
 * offset is the starting offset of the I/O in bytes,
 * length is the length of the I/O in bytes.
 */

//temp fun
//int Log_Init(char *filename, Inode **ifile, int *closestatus);


//contains direct block info in disk
typedef struct Block_pointer
{
    //------- file bk size in sectors--------
    //------actually in flash memory: 1 sector is its 1 block-------
    int seg_no;
    int bk_no;
}Block_pointer;


typedef struct Inode
{
    u_int ino;
   
//    u_int seg_no_in_log;//this inode is stored in which seg of log
//    u_int bk_no_in_log;//this inode is stored in which block of log
    u_int filetype;
    u_int filesize;
    char filename[FILE_NAME_LENGTH + 1]; //phase 1
    
    //flash address of the inode's blocks
    Block_pointer direct_bk[DIRECT_BK_NUM];
    //Bk_list indirect_bk;
    //Block_pointer indirect_bk;  phase 2
   
    mode_t mode;
    uid_t userID;
    gid_t groupID;
    time_t modify_Time;
    time_t access_Time;
    time_t create_Time;
    time_t change_Time;
    int num_links;

}Inode;

//store the inode 
typedef struct Inode_location
{
    Inode inode;
    //offset in "ifile",
    // there stores the inode for ino
    u_int offset; 
    struct Inode_location * next;

}Inode_location;

int File_Init(Inode *inode, int type);

int File_Write(Inode *inode, int offset, int length, void *buffer);

int File_Read(Inode *inode, int offset, int length, void *buffer);

int File_Free(Inode *inode);

int File_Layer_Init(char *filename, Inode **ifile);

void Get_Block_pointer(Inode *inode, int ReadBlockNumber, Block_pointer *blockpointer);
