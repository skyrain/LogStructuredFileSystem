#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "dir.h"
//-----------define the metadata -------

Seg_sum_entry *segSumEntry;
Seg_sum_bk *segSumBk;
LogAddress *LogAddr;
Super_seg *Super_seg;

//segment usage, block usage 
Seg_usage_table *SegUsageTable;
Seg_sum_entry *SegBlockTable;

#define DEBUG FALSE

//store the status of used Inode whether it has been checked
int *usedInodes;
int numUsedInodes;
int lengthIFile;
extern seg_num;
extern bk_per_seg;

static void print_usage(){
	printf( "USAGE\n" );
}

int Check_Block(int inum, int file_blocknum, Block_pointer *blockPointer){

	int segnum, blocknum;
	segnum = blockPointer->seg_no;
	blocknum = blockPointer->bk_seg;
	
	int i;
	for(i=1; i<segnum; i++)
	{
		//Find the number segnum table
		SegUsageTable = SegUsageTable->next
		SegBlockTable = SegBlockTable->next
	}
	
	if (segnum >=  seg_num ){
		printf("Inum %i: ERROR in file.\n", inum);
		printf("    Block %i is in segment %i but there are only %i segments in the disk\n",
					file_blocknum, segnum, _thisLog.metadata.numsegments);
		return 1;
	} else if (segnum >= 0){
		if(blocknum >= bk_per_seg){
			printf("Inum %i: ERROR in file.\n", inum);
			printf(" Error :Block %i is in segment %i block %i but there are only %i blocks in a segment.\n",
					file_blocknum, segnum, blocknum, _thisLog.metadata.segmentsize);
			return 1;
		//check the blockUsage, file_no and inum, file block num
		// compare log addr with
		}else if (SegBlockTable->file_no != inum ||
			SegBlockTable->file_bk_no != file_blocknum){
			printf("Inum %i: ERROR in file.\n", inum);
			printf("Inode: Block %i is in Seg %i Block %i\n",
					file_blocknum, segnum, blocknum);
			printf("    SegMetaData: Seg %i Block %i belongs to file %i block %i\n",
					segnum, blocknum, SegBlockTable->file_no,
					SegBlockTable->file_bk_no);
			return 1;
		}else{
			// This block is correct, update seg usage table
			printf("Incrementing segusage table segment %i.\n", segnum);
			SegUsageTable->num_live_bk ++;
			// Need to update other data?
		}
	}

	return 0;
}

int Check_File(Inode *myNode){
	// Checks the file specified by myNode for consistency
	int j, numiblocks , i;
	int status = 0;
	//BlockPointer *indirectblock;
	
	// Mark this Inode as used
	if (myNode->ino >= 0){
		// if a ifile has a negative ino, so ignore it
		if (usedInodes[myNode->ino] >= 1){
			// This Inode has already been checked, return now
			return 0;
		}else{
			usedInodes[myNode->ino] = 1; // marked this Inode been check
		}
	}
	printf("Checking file %i...\n", myNode->ino);

	// Check that the segments/blocks in the Inode match the
	// segment meta data
	// loop over all of the direct links for this file
	for (j = 0; j < DIRECT_BK_NUM; j ++){
		printf("Inum %i: Checking block %i.\n", myNode->ino, j);
		status += Check_Block(myNode->ino, j, &myNode->direct_bk[j]);
	}

	return status;
}

// ----????---- in phase 2, we need check the validation of directory structure.

int main(int argc, char *argv[])
{
	char	*Flashname; // Name of file that is our disk
	int		status, i;
	int sizeBUT; // holds the size in bytes of the block usage table
	Inode *rootNode;
	char *block_buff; // Can hold one block of data
	Inode *myNode; // This buffer will hold the ifile
	Inode *ifile
	
	//get the super segment init ifile and seg_num.
	Seg *SuperSeg;
	copy_log_to_memory(0, *SuperSeg)
	seg_num = SuperSeg->seg_num;
	
	ifile = SuperSeg -> checkpoint -> ifile;
	
	 // one required argument: (first arg is executable name)
	 //	-filename for the flash file
	
	// read the Flashname from the argument
	if( argc < 2){
		print_usage();
		return EXIT_FAILURE;
	} else {
		Flashname = argv[1];
	}
	
	// Not need to init in this way, I cannot handle that
	/*
	status = Dir_Layer_Init(Flashname, 5); //set cache size to 5 - it shouldn't matter
	if (status){
		return status;
	}
	*/
	
	// ---???--- ifile
	if (ifile.filesize == 0){
		printf("LFS not fully initialized. Please run LFS before checking.\n");
		return 1;
	}
	
	// Another functions ??
	// Allocate the block buffer

	// Get the root Inode
	status = _Get_Inode_From_Inum(ROOT_INUM, &rootNode);
	if (status){
		printf("ERROR getting root Inode.\n");
		return status;
	}

	// Check the ifile
	printf("Checking ifile...\n");
	status = Check_File(ifile);
	if (status){
		printf("ERROR in ifile.\n");
	}

	lengthIFile = ifile.filesize / (sizeof(Inode));
	usedInodes = (int *) calloc(sizeof(int), lengthIFile);
	numUsedInodes = 0;


	// ---???--- Check the directory structure in phase 2
	
	// Check segment usage table
	// Initialize the segment usage table
	
	Seg_usage_table *SegUsageTable;
	Seg_sum_entry *SegBlockTable;

	SegUsageTable = (Seg_usage_table *)calloc(1,sizeof(Seg_usage_table));
	SegBlockTable = Super_seg -> seg_usage_table;

	/*
	printf("Checking the segment usage table...\n");
	{
		// get the current segment block table with set_num.
		if (SegBlockTable -> num_live_bk != segUsageTable -> numBlocksUsed){
			printf("ERROR in segment usage table for segment %i.\n", i);
			printf("  Reconstructed number of blocks in used : %i blocks used.\n", SegUsageTable -> num_live_bk);
			printf("  Log SUT:           %i blocks used.\n", SegBlockTable -> num_live_bk);
			status = 1;
		}
	}
	*/
	
	// Check ifile for Inode consistency
	printf("Checking that all Inodes in ifile exist in directory structure...\n");
	for (i = 0; i < lengthIFile; i++){
		status = Get_Inode_From_Inum(i, &myNode);
		if (status){
			printf("ERROR: Could not get Inode for inum %i\n", i);
			return status;
		}
		if (myNode->mode > 0 && usedInodes[i] == 0){
			printf("ERROR: Inode %i exists in ifile but does not appear in the directories.\n", i);
		}
	}

	//status = Open_Log( Flashname, &log, printing );

	// done, shut down, undone
	//Dir_Layer_Denit();

	free(SegUsageTable);
	free(SegBlockTable);

	return status;
}
