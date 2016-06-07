#ifndef _PROCESS_SCHEDULING_HPP_
#define _PROCESS_SCHEDULING_HPP_

#include <dyn_array.h>
#include <stdbool.h>

typedef struct {
	uint32_t burstTime; // CPU runtime for Process
	uint32_t arrivalTime; // The time the process comes into the scheduler
	int32_t pid; // Used to bind a running process with a PCB
	uint32_t activated; // Finally reached the CPU
} ProcessControlBlock_t;

typedef struct {
	float averageWallClockTime; // The average amount of time to complete a process
	float averageLatencyTime; // The average delay between being activated on the cpu from the ready queue
	size_t totalClockRuntime; // The final time of all processes
} ScheduleStats_t;

const ScheduleStats_t first_come_first_served(dyn_array_t* futureProcesses);
const ScheduleStats_t shortest_job_first(dyn_array_t* futureProcesses);
const ScheduleStats_t shortest_remaining_time_first(dyn_array_t* futureProcesses);
const ScheduleStats_t round_robin(dyn_array_t* futureProcesses, const size_t quantum); 

/*Helper Function*/
void virtual_cpu(ProcessControlBlock_t* runningProcess); 
const bool load_process_control_blocks_from_file(dyn_array_t* futureProcesses, const char* binaryFileName);
const bool create_suspended_processes_and_assign_pcbs(dyn_array_t* futureProcesses);
const bool rearranged_process_control_blocks_by_arrival_time(dyn_array_t* futureProcesses); 
const bool fetch_new_processes(dyn_array_t* newProcesses, dyn_array_t* futureProcesses, const size_t currentClockTime); 
//Compare function used to sort pcbS by arrival time from greatest to least
int compare(const void* a, const void *b);
//Compare function used to sort pcbs by burst time from least to greatest
int compareBurstTime(const void* a, const void*b);
#endif



