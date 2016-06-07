#include <stdio.h>
#include <stdlib.h>
#include <dyn_array.h>
#include <string.h>
#include <math.h>

#include "../include/process_scheduling.h"

#define PASS_OK printf("[%d]\t%s\n",(++TESTS),"."); 
#define PASS_NOTOK(line) printf("[%d]\t%s\tline(%d)\n",(++TESTS),"FAIL",line);
#define assert(x,line) if(x) PASS_OK else PASS_NOTOK(line);


void test_load_process_control_blocks_from_file();
void test_create_suspended_processes_and_assign_pcbs();
void test_rearranged_process_control_blocks_by_arrival_time();
void test_fetch_new_processes();

void run_FCFS_test ();
void run_SJF_test ();
void run_SRTF_test();
void run_RR_test();

int TESTS = 0;

int main (int argc, char** argv) {
	
	if (argc < 2) {
		fprintf(stdout,"Program needs at least 2 parameters to run\n");
		return -1;
	}
	
	char* pEnd = NULL;
	const size_t scheduleAlgorithm = strtol(argv[1],&pEnd,10);
	if (scheduleAlgorithm < 0 || scheduleAlgorithm > 4) {
		printf("SCHEDULE ALGORITHMS:\n\n \
				1)FIRST COME FIRST SERVED\n \
				2)SHORTEST JOB FIRST\n \
				3)SHORTEST REMAINING TIME FIRST\n \
				4)ROUND ROBIN\n");
		return -1;
	}

	/*
	* Test the setup functions before scheduling processes 
	**/
	test_load_process_control_blocks_from_file();
	test_create_suspended_processes_and_assign_pcbs();
	test_rearranged_process_control_blocks_by_arrival_time();
	test_fetch_new_processes();	
	/*
	* PRE-TESTS FOR VERIFICATION OF CORRECTNESS
	*
	**/
	switch(scheduleAlgorithm) {
		case 1:
			run_FCFS_test();
		break;
		case 2:
			run_SJF_test();
		break;
		case 3:
			run_SRTF_test();
		break;
		case 4:
			run_RR_test();
		break;
	}

	return 0;

}

void test_load_process_control_blocks_from_file() {
	dyn_array_t* tester = dyn_array_create(16,sizeof(ProcessControlBlock_t),NULL);
	
	// Test bad parameters
	assert((load_process_control_blocks_from_file(NULL,NULL) == false),__LINE__);
	assert((load_process_control_blocks_from_file(NULL,"\0") == false),__LINE__);
	assert((load_process_control_blocks_from_file(tester,"Data/WrongPCBs.bin") == false),__LINE__);
	assert((dyn_array_size(tester) == 0),__LINE__);

	//Test good parameters
	assert(load_process_control_blocks_from_file(tester,"../data/SamplePCBs.bin") == true,__LINE__);
	assert((dyn_array_size(tester) == 3),__LINE__);
	dyn_array_destroy(tester);

}

void test_create_suspended_processes_and_assign_pcbs() {
	
	ProcessControlBlock_t data[3] = {
		[0] = {10,0,0,0},
		[1] = {12,1,0,0},
		[2] = {13,2,0,0}
	};
	dyn_array_t* tester = dyn_array_import(data,3,sizeof(ProcessControlBlock_t),NULL);
	assert((create_suspended_processes_and_assign_pcbs(NULL) == false),__LINE__);
	
	for (size_t i = 0; i < 3; ++i) {
		ProcessControlBlock_t* pcb = dyn_array_at(tester,i);
		assert((!pcb->pid),__LINE__);

	}

	assert((create_suspended_processes_and_assign_pcbs(tester) == true),__LINE__);
	for (size_t i = 0; i < 3; ++i) {
		ProcessControlBlock_t* pcb = dyn_array_at(tester,i);
		assert((pcb->pid),__LINE__);
	}	
	
	dyn_array_destroy(tester);
}

void test_rearranged_process_control_blocks_by_arrival_time() {
		
	ProcessControlBlock_t data[3] = {
		[0] = {14,0,0,0},
		[1] = {12,1,0,0},
		[2] = {13,2,0,0}
	};
	
	ProcessControlBlock_t expected[3] = {
		[0] = {13,2,0,0},
		[1] = {12,1,0,0},
		[2] = {14,0,0,0}
	};


	dyn_array_t* tester = dyn_array_import(data,3,sizeof(ProcessControlBlock_t),NULL);
	assert(rearranged_process_control_blocks_by_arrival_time(NULL) == false,__LINE__);
	
	for (size_t i = 0; i < 3; ++i) {
		ProcessControlBlock_t* pcb = dyn_array_at(tester,i);
		assert(pcb->arrivalTime == data[i].arrivalTime,__LINE__);
	}

	assert(rearranged_process_control_blocks_by_arrival_time(tester),__LINE__);
	for (size_t i = 2; i > 0; --i) {
		ProcessControlBlock_t* pcb = dyn_array_at(tester,i);
		assert(pcb->arrivalTime == expected[i].arrivalTime,__LINE__);
	}	
	
	dyn_array_destroy(tester);

}

void test_fetch_new_processes() {
	ProcessControlBlock_t data1[3] = {
			[0] = {50,0,0,0},
			[1] = {12,0,0,0},
			[2] = {5,0,0,0}
	};
	
	ProcessControlBlock_t data2[3] = {
			[0] = {24,0,0,0},
			[1] = {3,50,0,0},
			[2] = {3,5,0,0}
	};

	ProcessControlBlock_t data3[3] = {
			[0] = {24,6,0,0},
			[1] = {6,6,0,0},
			[2] = {6,6,0,0}
	};
	const size_t currentClockTime = 5;
	// Bad input parameters
	assert(fetch_new_processes(NULL,NULL,0) == false,__LINE__);
	// logic edge tests
	dyn_array_t* newProcesses = dyn_array_create(16,sizeof(ProcessControlBlock_t),NULL); 
	dyn_array_t* futureProcesses = dyn_array_import(data1,3,sizeof(ProcessControlBlock_t), NULL);  
	assert(fetch_new_processes(newProcesses,futureProcesses,0) == true,__LINE__);
	assert(dyn_array_size(newProcesses) == 3,__LINE__); 
	assert(dyn_array_size(futureProcesses) == 0,__LINE__);
	dyn_array_destroy(futureProcesses);
	dyn_array_clear(newProcesses);

	futureProcesses = dyn_array_import(data2,3,sizeof(ProcessControlBlock_t),NULL); 
	assert(fetch_new_processes(newProcesses,futureProcesses,currentClockTime) == true,__LINE__);
	assert(dyn_array_size(newProcesses) == 2,__LINE__);
	assert(dyn_array_size(futureProcesses) == 1, __LINE__);
	dyn_array_destroy(futureProcesses);
	dyn_array_clear(newProcesses);

	futureProcesses = dyn_array_import(data3,3,sizeof(ProcessControlBlock_t),NULL); 
	assert(fetch_new_processes(newProcesses,futureProcesses,currentClockTime) == false,__LINE__);
	assert(dyn_array_size(newProcesses) == 0,__LINE__);
	assert(dyn_array_size(futureProcesses) == 3,__LINE__);
	dyn_array_destroy(futureProcesses);
	dyn_array_destroy(newProcesses);
}


void run_FCFS_test () {
	ScheduleStats_t stats = first_come_first_served(NULL);
	assert(stats.averageWallClockTime == 0,__LINE__);
	assert(stats.averageLatencyTime == 0,__LINE__);
	dyn_array_t* tester = dyn_array_create(16,sizeof(ProcessControlBlock_t),NULL);
	stats = first_come_first_served(tester);
	assert(stats.averageWallClockTime == 0,__LINE__);
	assert(stats.averageLatencyTime == 0,__LINE__);

	ProcessControlBlock_t data[3] = {
			[0] = {24,0,0,0},
			[1] = {3,0,0,0},
			[2] = {3,0,0,0}
	};
	dyn_array_push_back(tester,&data[0]);
	dyn_array_push_back(tester,&data[1]);
	dyn_array_push_back(tester,&data[2]);
	create_suspended_processes_and_assign_pcbs(tester);
	stats = first_come_first_served(tester);
	float answers[2] = {27,17};
	assert(stats.averageWallClockTime == answers[0],__LINE__);
	assert(stats.averageLatencyTime == answers[1],__LINE__); 
	printf("AVG WALL CLOCK TIME = %f\nAVG LATENCY TIME = %f\n", stats.averageWallClockTime, stats.averageLatencyTime);
	dyn_array_destroy(tester);
}



void run_SJF_test () {
	ScheduleStats_t stats = shortest_job_first(NULL);
	assert(stats.averageWallClockTime == 0,__LINE__);
	assert(stats.averageLatencyTime == 0,__LINE__);
	dyn_array_t* tester = dyn_array_create(16,sizeof(ProcessControlBlock_t),NULL);
	stats = shortest_job_first(tester);
	assert(stats.averageWallClockTime == 0,__LINE__);
	assert(stats.averageLatencyTime == 0,__LINE__);
	dyn_array_clear(tester);

	ProcessControlBlock_t data[4] = {
			[0] = {6,0,0,0},
			[1] = {8,0,0,0},
			[2] = {7,0,0,0},
			[3] = {3,0,0,0}
	};
	dyn_array_push_back(tester,&data[0]);
	dyn_array_push_back(tester,&data[1]);
	dyn_array_push_back(tester,&data[2]);
	dyn_array_push_back(tester,&data[3]);
	create_suspended_processes_and_assign_pcbs(tester);
	stats = shortest_job_first(tester);
	float answers[2] = {13,7};
	assert(stats.averageWallClockTime == answers[0],__LINE__);
	assert(stats.averageLatencyTime == answers[1],__LINE__);
	printf("AVG WALL CLOCK TIME = %f\nAVG LATENCY TIME = %f\n", stats.averageWallClockTime, stats.averageLatencyTime);
	dyn_array_destroy(tester);

}

void run_SRTF_test () {
	ScheduleStats_t stats = shortest_remaining_time_first(NULL);
	assert(stats.averageWallClockTime == 0,__LINE__);
	assert(stats.averageLatencyTime == 0,__LINE__);
	dyn_array_t* tester = dyn_array_create(16,sizeof(ProcessControlBlock_t),NULL);
	stats = shortest_remaining_time_first(tester);
	assert(stats.averageWallClockTime == 0,__LINE__);
	assert(stats.averageLatencyTime == 0,__LINE__);
	dyn_array_clear(tester);
	
	ProcessControlBlock_t data[4] = {
			[3] = {8,0,0,0},
			[2] = {4,1,0,0},
			[1] = {9,2,0,0},
			[0] = {5,3,0,0}
	};
	dyn_array_push_back(tester,&data[0]);
	dyn_array_push_back(tester,&data[1]);
	dyn_array_push_back(tester,&data[2]);
	dyn_array_push_back(tester,&data[3]);
	create_suspended_processes_and_assign_pcbs(tester);
	stats = shortest_remaining_time_first(tester);
	float answers[2] = {13,4.25};
	assert(stats.averageWallClockTime == answers[0],__LINE__);
	assert(stats.averageLatencyTime == answers[1],__LINE__);
	printf("AVG WALL CLOCK TIME = %f\nAVG LATENCY TIME = %f\n", stats.averageWallClockTime, stats.averageLatencyTime);
	dyn_array_destroy(tester);

}

void run_RR_test () {		
	ScheduleStats_t stats = shortest_remaining_time_first(NULL);
	assert(stats.averageWallClockTime == 0,__LINE__);
	assert(stats.averageLatencyTime == 0,__LINE__);
	dyn_array_t* tester = dyn_array_create(16,sizeof(ProcessControlBlock_t),NULL);
	stats = shortest_remaining_time_first(tester);
	assert(stats.averageWallClockTime == 0,__LINE__);
	assert(stats.averageLatencyTime == 0,__LINE__);
	tester = dyn_array_create(16,sizeof(ProcessControlBlock_t),NULL);
	ProcessControlBlock_t data[3] = {
			[0] = {24,0,0,0},
			[1] = {3,0,0,0},
			[2] = {3,0,0,0}
	};
	dyn_array_push_back(tester,&data[0]);
	dyn_array_push_back(tester,&data[1]);
	dyn_array_push_back(tester,&data[2]);
	create_suspended_processes_and_assign_pcbs(tester);
	stats = round_robin(tester, 4);
	float answers[2] = {15.666667,3.666667};
	assert(stats.averageWallClockTime == answers[0],__LINE__);
	assert(fabs(stats.averageLatencyTime - answers[1]) < .00001,__LINE__);
	printf("AVG WALL CLOCK TIME = %f\nAVG LATENCY TIME = %f\n", stats.averageWallClockTime, stats.averageLatencyTime);
	dyn_array_destroy(tester);

}
