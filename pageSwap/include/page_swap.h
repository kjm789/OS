#ifndef _PAGE_SWAP_H_
#define _PAGE_SWAP_H_

#include <stdbool.h>

#define MAX_PAGE_TABLE_ENTRIES_SIZE 2048
#define MAX_PHYSICAL_MEMORY_SIZE 512
/***
PURPOSE: struct to represent an instance of a frame
***/
typedef struct {
	unsigned int pageTableIdx;
	unsigned char data[1024];
	unsigned char accessTrackingByte;
	unsigned char accessBit;
}Frame_t;
/***
PURPOSE: struct to represent frameTable; Holds an array of frames
***/
typedef struct {
	Frame_t entries[MAX_PHYSICAL_MEMORY_SIZE];
	unsigned int size;
}FrameTable_t;
/***
PURPOSE: struct to represent an instance of a page
***/
typedef struct {
	unsigned int frameTableIdx;
	bool val; //Validity ==> valid(true) || unvalid(false)
}Page_t;

/***
PURPOSE: struct to represent pageTable; Holds an array of pages
***/
typedef struct {
	Page_t entries[MAX_PAGE_TABLE_ENTRIES_SIZE]; // An array of pages
	unsigned int size; // Number of Page entires
}PageTable_t;
/***
PURPOSE: struct to represent relevant info when a page fault occurs
***/
typedef struct {
	unsigned short pageRequested;
	unsigned short frameReplaced;
	unsigned short pageReplaced;
}PageAlgorithmResults;
/***
PURPOSE: To initialize the backStore for our paging in and paging out functionality; liblock_store is used for this
INPUT: none
OUTPUT: validity of function(worked = true :: failed = falsed
***/
bool initialize_back_store (void);
/***
PURPOSE: To destory the backStore for our paging in and paging out functionality; deallocate BS memory
INPUT:NONE 
OUTPUT:NONE 
***/
void destroy_back_store(void);
/***
PURPOSE: To initialize a dyn_array_t that will stand as frame index list; this servers for LRU finding the victim page or LRU page 
INPUT: NONE 
OUTPUT: validity of function(worked = true :: failed = falsed
***/
bool initailize_frame_list(void);
/***
PURPOSE: To destroy a dyn_array_t that will stand as frame index list; to deallocate dyn_array_t memory
INPUT: none 
OUTPUT: none
***/
void destroy_frame_list(void);
/***
PURPOSE: To determine if from the page request if a page fault will occurr; if it does we will have to find a the LRU page, writing this
	to BS and reading the page data that will now be in memory....such we are updating out tables with correct info
INPUT: pageNumber ==> pageNumber of the page request being made
OUTPUT: returns relevant info of transaction; if no page fault, OUTPUT will be NULL, if page fault info of transaction will be returned
***/
PageAlgorithmResults* least_recently_used(const uint32_t pageNumber);
/***
PURPOSE: To approximate the LRU page given a accessPattern, also verifying accessBit to see if use or not
INPUT: pageNumber ==> pageNumber of the page request being made
OUTPUT: returns relevant info of transaction; if no page fault, OUTPUT will be NULL, if page fault info of transaction will be returned
***/
PageAlgorithmResults* approx_least_recently_used (const uint32_t pageNumber, const size_t timeInterval);
/***
PURPOSE: To read from BS...we are reading in the data of the requested page that was not in memory
INPUT: pageNum ==> page that is being requested; frameNum => frame of which we will read into from BS
OUTPUT: validity of function(worked = true :: failed = falsed
***/
bool read_from_back_store (const uint32_t pageNum, const int frameNum);
/***
PURPOSE: To write to BS....we are writing data from frame we will take out of memory and store on the BS
INPUT: frameNum ==> frameNum of which we will use to its data to BS 
OUTPUT: validity of function(worked = true :: failed = falsed
***/
bool write_to_back_store (const int frameNum);
/***
PURPOSE: Reading in the requested Pages from binary file
INPUT: filename==> name of file we will be reading from
OUTPUT: dyn_array_t* ==> array where our page request will live 
***/
dyn_array_t* read_page_requests (const char* const filename);
/***
PURPOSE: To initialize both PageTable and FrameTable; This will be a one to one mapping initially. We will also initialize the frameIdxList which will give us the LRU page when page fault occurs
INPUT: None
OUTPUT: validity of function(worked = true :: failed = falsed
***/
bool initialize (void);
/***
PURPOSE: We will seek the pageTable, using pageReq as a index in PT
INPUT: pageReq ==> number of page request being made
OUTPUT: validity of page in memory (valid == In memory || invalid == Not in memory
***/
bool seekPageTable(const uint32_t pageReq);
/***
PURPOSE: We will use this to seek the frameTable, to find the page that is valid in memory, and returning the index to use in frameIdxList
INPUT: pageNum ==> requested page that was found to be in memory
OUTPUT: index of the that page thus we can use for the frameIdxList
***/
uint32_t seekFrameTable(const uint32_t pageNum);
/***
PURPOSE: We use this function when a page request had been found to be in memory, thus updating the frameListList....extract that particalur frameIdx and pushing this on back....front of frameIdxList will always have the LRU page 
INPUT: frameIdx ==> the index of the FT of which where the page requested lives
OUTPUT: validity of function(worked = true :: failed = falsed
***/
bool extract_pushBack_frameIdxList(const uint32_t frameIdx);
/***
PURPOSE: We use this function to extract a page from the front of frameIdxList (LRU page), thus pushing it on the back of the list
INPUT: NONE 
OUTPUT: int==> represents the frameIdx in FT of the LRU page 
***/
uint32_t extractVictim_pushBack_frameIdxList();
/***
PURPOSE: to set page invalid or not in memory 
INPUT: frame ==> frameNum to use to reference in PT the page that was swapped out
OUTPUT: NONE 
***/
void setPage_inval(const uint32_t frameNum);
/***
PURPOSE: to set page to valid or into memory, and set the frameTableIdx with the frameNum
INPUT: pageNum ==> page number that was requeted used to reference that page in the PT; frameNum ==> new index of the where this page will live in the FT
OUTPUT: none
***/
void setPage_val(const uint32_t pageNum, const uint32_t frameNum);
/***
PURPOSE: To find the minimum AccessPattern in the frameTable...we do this by comparing accessTrackingByte with the MSB, which in this case is 128 
INPUT: none
OUTPUT: will return the frameNum of which is the frame with the lowest accessPattern in the FT...being this refers to the frame holding the approx LRU page 
***/
uint32_t argMin();
#endif
