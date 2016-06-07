#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include <dyn_array.h>


#include "../include/page_swap.h"

int main (int argc, char **argv) {

	if (argc < 2) {
		printf("Not Enough Parameters for %s\nCorrect Usage is: %s <page swap request binary file>\n\n", argv[0], argv[0]);
		return -1;
	}

	PageAlgorithmResults* results = NULL;	
	dyn_array_t* pageRequests = NULL;
	pageRequests = read_page_requests (argv[1]);
	if (!pageRequests) {	
		printf("ERROR: FAILED TO FILL IN PAGE REQUESTS\n");
		return -1;
	}

	if(!initialize_back_store ()){
		printf("ERROR: FAILED TO INIT BACK STORE");
		return -1;
	}


	if(!initailize_frame_list()) {
		printf("ERROR: FAILED TO INIT FRAME LIST");
		return -1;	
	}
	
	/*
	 * TESTING LRU PAGE SWAP ALGORITHM
	 **/

	printf("LRU PAGE SWAP ALGORITHM\n");
	printf("------------------------------------\n");

	if(!initialize()) {
		printf("ERROR: FAILED TO INIT FRAME TABLE AND PAGE TABLE\n\n");
		return -1;
	}
	
	const size_t size = dyn_array_size(pageRequests);
	size_t pageFaults = 0;
	for (int i = 0; i < size; ++i) {
		uint32_t* pageNumber = (uint32_t*) dyn_array_at(pageRequests,i);
		results = least_recently_used (*pageNumber);
		if(results) {
			printf("%u)PAGE FAULT: Page Requested = %u, Frame Replaced = %u, Page Replaced = %u\n",i,
					results->pageRequested,results->frameReplaced,results->pageReplaced);
			free(results);
			results = NULL;
			pageFaults++;
		}
		else {
			printf("NOT PAGE FAULT OCCURED\n\n");

		}
	}

	printf("\n\nTotal Page Faults = %lu\n", pageFaults);

	printf("------------------------------------\n");

	/*
	 * TESTING LRU APPOX PAGE SWAP ALGORITHM
	 **/
	printf("APPROX LRU PAGE SWAP ALGORITHM\n");
	printf("------------------------------------\n");

	if(!initialize()) {
		printf("ERROR: FAILED TO INIT FRAME TABLE AND PAGE TABLE\n\n");
		return -1;
	}
	pageFaults = 0;
	for (int i = 0; i < size; ++i) {
		uint32_t* pageNumber = (uint32_t*) dyn_array_at(pageRequests,i);
		results = approx_least_recently_used (*pageNumber,i);
		if(results) {
			printf("%u)PAGE FAULT: Page Requested = %u, Frame Replaced = %u, Page Replaced = %u\n",i,
					results->pageRequested,results->frameReplaced,results->pageReplaced);
			free(results);
			results = NULL;
			pageFaults++;
		}
		else {
			printf("NOT PAGE FAULT OCCURED\n\n");
		}
	}

	printf("\n\nTotal Page Faults = %lu\n", pageFaults);
	printf("------------------------------------\n");

	destroy_back_store();
	destroy_frame_list();
	dyn_array_destroy(pageRequests);
	return 0;
}
