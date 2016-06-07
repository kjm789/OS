#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


#include <dyn_array.h>
#include <block_store.h>

#include "../include/page_swap.h"

/*
 * Global protected variables
 **/
static FrameTable_t frameTable;
static PageTable_t pageTable;
static dyn_array_t* frameIdxList;
static block_store_t* blockStore;

/*
 * CLEARS THE PAGE TABLE AND FRAME TABLE WITH ALL ZEROS
 **/
bool initialize_back_store (void) {
	blockStore = block_store_create();
	if (!blockStore) {
		return false;
	}
	return true;
}
//destory our backStore, or BS from block_store
void destroy_back_store(void) {
	block_store_destroy(blockStore, BS_NO_FLUSH);
}
//init frameIdxList of which will tell us the LRU page
bool initailize_frame_list(void) {
	frameIdxList = dyn_array_create(512,sizeof(unsigned int),NULL);
	if (!frameIdxList) {
		return false;
	}
	return true;
}
//destory dyn_array_t used for LRU
void destroy_frame_list(void) {
	dyn_array_destroy(frameIdxList);
}

/*
 * TODO COMPLETE ALL THE FOLLOWING FOR EACH FUNCTION: LOGIC and DOCUMENTATION 
 */
PageAlgorithmResults* least_recently_used(const uint32_t pageNumber) {
	
	PageAlgorithmResults* pageResults = NULL;
	//if condition true==> no page fault
	if( seekPageTable(pageNumber) ) {
		uint32_t frameIdx = seekFrameTable(pageNumber); // find the frameIdx where pageReq'd # lives
	
		if( !(extract_pushBack_frameIdxList(frameIdx)) )  
			return NULL;
		return NULL;
	}
	else { //else page fault has occurred
		uint32_t frameNum = extractVictim_pushBack_frameIdxList();
		pageResults = (PageAlgorithmResults*)malloc(sizeof(PageAlgorithmResults));
		//swapping out
		if( !(write_to_back_store(frameNum)) ) {
			return NULL;
		}	
		setPage_inval(frameNum);
		//swapping in
		setPage_val(pageNumber, frameNum);
		uint32_t buff = frameTable.entries[frameNum].pageTableIdx;
		pageResults->pageReplaced = buff;
		frameTable.entries[frameNum].pageTableIdx = pageNumber;//set the new swapped in page to FT
		//swapping in
		if( !(read_from_back_store(pageNumber, frameNum)) )
			return NULL;
		pageResults->pageRequested = pageNumber;	
		pageResults->frameReplaced = frameNum;
	}
	return pageResults;
}
//Love the accessPattern, this allows us to not depend on frameIdxList
PageAlgorithmResults* approx_least_recently_used (const uint32_t pageNumber, const size_t timeInterval) {	
	PageAlgorithmResults* pageResults = NULL;
	uint32_t frameNum, i, buffer;
	if(pageTable.entries[pageNumber].val)
		return NULL;
	else {
		unsigned char msb = 128;
		pageResults = (PageAlgorithmResults*)malloc(sizeof(PageAlgorithmResults));
		if(!pageResults)
			return NULL;
		//find the minimum accessTrackingByte, where this will refer to the frameNum of the approx LRU pageNum
		frameNum = argMin();
		//fill pageResults with good/relevant info
		buffer = frameTable.entries[frameNum].pageTableIdx; 
		pageResults->pageRequested =  pageNumber;
	   	pageResults->frameReplaced = frameNum; 
		pageResults->pageReplaced = buffer;	/*frameTable.entries[frameNum].pageTableIdx*/
		//swapping out
		if( !(write_to_back_store( frameNum)) )
			return NULL;
		setPage_inval(frameNum);
		//swapping in
		setPage_val(pageNumber, frameNum);
		frameTable.entries[frameNum].pageTableIdx = pageNumber;
		if( !(read_from_back_store(pageNumber, frameNum)) )
			return NULL;
		//set accessBit of swapped in page to 1, or accessed
		frameTable.entries[ pageTable.entries[pageNumber].frameTableIdx ].accessBit = 1;
		if(timeInterval % 99 == 0) { //let the magic happen
			for(int i = 0; i < 512; ++i){
				frameTable.entries[i].accessTrackingByte >>= 1;
				frameTable.entries[i].accessTrackingByte |= msb;
			}
		}		
	}
	return pageResults;
}
//read page date we are swapping from BS
bool read_from_back_store (const uint32_t pageNum, const int frameNum) {

	if( !(block_store_read(blockStore, (pageNum + 8), frameTable.entries[frameNum].data, 1024, 0)) )
		return false;
	return true;
}
//write page data we are swapping out to BS 
bool write_to_back_store (const int frameNum) {
	
	if( (block_store_write(blockStore, ( (frameTable.entries[frameNum].pageTableIdx) + 8 ), frameTable.entries[frameNum].data, 1024, 0)) != 1024 )
		return false;
	else
		return true;
}
//read binary file of page request
dyn_array_t* read_page_requests ( const char* const filename) {
	
	if(!filename) //Error Check incoming parameter(s)
		return NULL;
	int fd = open(filename, O_RDONLY);//Read only for the binary formatted file
	if(fd == -1)//Checking open() worked correctly
		return NULL;
	uint32_t numReq = NULL;
	if(!(read(fd, &numReq, sizeof(uint32_t)))){//Checking that read() worked correctly
		close(fd);
		return NULL;
	}
	//dyn_array for the page Request
	dyn_array_t* page_Req = dyn_array_create(numReq, sizeof(unsigned int), NULL);
	uint32_t page = NULL;
	int i = 0;
	//read from binary fill && push back on page_Req
	for(i = 0; i < numReq; ++i){
		if(!(read(fd, &page, sizeof(uint32_t)))){//Error check 
			close(fd);
			return NULL;
		}
		if(!(dyn_array_push_back(page_Req, &page))){//Error check
		   close(fd);
		   return NULL;
		}		
	}
	return page_Req;
}

bool initialize (void) {
	
	/*zero out my tables*/
	memset(&frameTable,0,sizeof(FrameTable_t));
	memset(&pageTable,0,sizeof(PageTable_t));

	/* Fill the Page Table from 0 to 512*/
	for (int i = 0; i < 512; ++i) {
		pageTable.entries[i].frameTableIdx = i;
		pageTable.entries[i].val = true;	
		(pageTable.size)++;		
	}	
	/* Fill the entire Frame Table with correct values*/
	for ( int i = 0; i < 512; ++i ) {
		frameTable.entries[i].pageTableIdx = i;
	 	if( !(dyn_array_push_front(frameIdxList, &i)) )
			/*throw error*/
			return false;
		(frameTable.size)++;	
	}
	//if size is zero in either table or FT.size does not equal PT.size, error occurred init Tables
	if( ( frameTable.size == 0 ||pageTable.size == 0) && frameTable.size != pageTable.size )
		/*throw error*/
		return false;
	else
		return true;
}
//look to see if pageReq is valid, or in memory
bool seekPageTable(const uint32_t pageReq) 
{
	return pageTable.entries[pageReq].val;
}
//find the frameIdx in FT thus we can use it in frameIdxList
uint32_t seekFrameTable(const uint32_t pageNum)
{
	for( uint32_t i = 0; i < 512; ++i ) {
		if(pageNum == frameTable.entries[i].pageTableIdx)
			return i;	
	}
}
//extract the for the page request that was in memory...updating this frameIdxList allows for the front to allows be the LRU page
bool extract_pushBack_frameIdxList(const uint32_t frameNum) 
{
	uint32_t buffer;
	uint32_t* frameIdx = NULL;

	for( int i = 0; i < 512; ++i ) {
		frameIdx = (uint32_t*)dyn_array_at(frameIdxList, i);
		if( frameNum == *frameIdx ) {
			dyn_array_extract(frameIdxList, i, &buffer);
			dyn_array_push_front(frameIdxList, &buffer); //push_front(...), which in this case is really the back, 	
			break;
		}
	}
	return true;
}

// get victim page....push to back of list....return that frameIdx such can be used to refernce page in FT
//  this is poping off of front such that frameIdx will use to reference to the Least Recently Used page
uint32_t extractVictim_pushBack_frameIdxList()
{
	uint32_t buffer;
	dyn_array_extract_back(frameIdxList, &buffer);
	dyn_array_push_front(frameIdxList, &buffer);
	return buffer;
}
//set page to invalid (no longer in memory)...page that is swapped out
void setPage_inval(const uint32_t frameNum)
{
	//before we update FT, we have to use FT to reference the pageTableIdx, using this to reference page in
   //  PT, updating val member to false
	pageTable.entries[ frameTable.entries[frameNum].pageTableIdx ].val = false;
}

//set page to valide...page that is swap in...then up PT where this page lives in FT
void setPage_val(const uint32_t pageNum, const uint32_t frameNum)
{
	pageTable.entries[pageNum].frameTableIdx = frameNum;// Have to know how to reference this page in FT, thus getting the FT	
	pageTable.entries[pageNum].val = true;				//  index where page lives, using this to upadte the frameIdxList
}
//find the minimum accessTrackingByte, thus returning that index of which is th frameNum that holds the pageNum
//	of which is the approx LRU page we will use to swap out and where we will swap in new requested page 
//After some initial test, I believe there is a bug in this function...run out of time for locating and fixing
uint32_t argMin()
{
	unsigned char msb = 128; //most significant bit 
	uint32_t min = NULL; //holds the index to the minimum accessTrackingByte

	for(uint32_t i = 0; i < 512; ++i)
	{
		if( frameTable.entries[i].accessTrackingByte < msb )
			min = i;
	}
	return min;
}
