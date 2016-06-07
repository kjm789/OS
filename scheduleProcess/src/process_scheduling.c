#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>

#include "../include/process_scheduling.h"
#include <dyn_array.h>


const ScheduleStats_t first_come_first_served(dyn_array_t* futureProcesses) {
	//create stats and initialize all to zero
	ScheduleStats_t stats;
	stats.averageWallClockTime = 0;
	stats.averageLatencyTime = 0;
	//error check incoming parameters && return stats if so
	if(!futureProcesses || dyn_array_empty(futureProcesses)){
		return stats;
	}
	//create readyQ, counter for completed processes, time counter, PCB_t to hold running Process
	dyn_array_t* readyQ = dyn_array_create(0, sizeof(ProcessControlBlock_t), NULL);
	size_t procDone = 0;
	size_t timer = 0;
	ProcessControlBlock_t runningProcess;

	//run loop until this condition, either readyQ is empty == all processes are done,
	// or futureProcesses is empty == all processes have been set to the readyQ,
	// or the current runningProcess is zero == all processes are completed 
	while(!dyn_array_empty(readyQ) || !dyn_array_empty(futureProcesses) || runningProcess.burstTime > 0){
		//if future processes is not empty, need to add them to the readyQ
		if(!dyn_array_empty(futureProcesses)){
			fetch_new_processes(readyQ, futureProcesses, timer);
		}
		//if timer==zero && dyn_array_empty(readyQ) not empty then need to extract from front of readyQ
		//or if runningProcess.burstTime == 0, then we need to kill process/add one to processes done/add time to stats
		if((timer == 0 && !dyn_array_empty(readyQ)) || runningProcess.burstTime == 0){
			if(timer != 0 && runningProcess.burstTime == 0){
				kill(runningProcess.pid, SIGKILL);
				++procDone;
				stats.averageWallClockTime += timer;
			}
			//if the readyQ is not empty and burstTime == 0 then we need to add the current wait time to Latency
			if(!dyn_array_empty(readyQ) && runningProcess.burstTime == 0){
				stats.averageLatencyTime += timer;
			}
			//error check for extract_front function; we are extracting new/first process from readyQ
			if(!dyn_array_extract_front(readyQ, &runningProcess))
				break;
		}
		//if readyQ is not empty or the running time of current process is greater than zero continue throwing current processes
		// onto virtualCPU
		if(!dyn_array_empty(readyQ) || runningProcess.burstTime > 0){
			virtual_cpu(&runningProcess);
		}
		++timer; //adding one to timer 
	}
	//destroy readyQ created && complete stats
	dyn_array_destroy(readyQ);
	stats.averageLatencyTime /= ++procDone;
	stats.averageWallClockTime += timer;
	stats.averageWallClockTime /= procDone;	
	return stats;
}


const ScheduleStats_t shortest_job_first(dyn_array_t* futureProcesses) {
	//create stats and initialize all to zero
	ScheduleStats_t stats;
	stats.averageWallClockTime = 0;
	stats.averageLatencyTime = 0;
	//error check incoming parameters && return stats if so
	if(!futureProcesses || dyn_array_empty(futureProcesses)){
		return stats;
	}
	dyn_array_t* readyQ = dyn_array_create(0, sizeof(ProcessControlBlock_t), NULL);
	size_t procDone = 0;
	size_t timer = 0;
	ProcessControlBlock_t runningProcess;
	//run loop until this condition, either readyQ is empty == all processes are done,
	// or futureProcesses is empty == all processes have been set to the readyQ,
	// or the current runningProcess is zero == all processes are completed 
	while(!dyn_array_empty(readyQ) || !dyn_array_empty(futureProcesses) || runningProcess.burstTime > 0){
		if(!dyn_array_empty(futureProcesses)){
			//if future processes is not empty, need to add them to the readyQ
			//if fetch_process fetchs process, adding to the readyQ, we need to 
			// sort readyQ by burstTime of process
			if(fetch_new_processes(readyQ, futureProcesses, timer)){
				dyn_array_sort(readyQ, &compareBurstTime);
			}
		}
		//if timer==zero && dyn_array_empty(readyQ) not empty then need to extract from front of readyQ
		//or if runningProcess.burstTime == 0, then we need to kill process/add one to processes done/add time to stats
		if((timer == 0 && !dyn_array_empty(readyQ)) || runningProcess.burstTime == 0){
			if(timer != 0 && runningProcess.burstTime == 0){
				kill(runningProcess.pid, SIGKILL);
				++procDone;
				stats.averageWallClockTime += timer;
			}
			//if the readyQ is not empty and burstTime == 0 then we need to add the current wait time to Latency
			if(!dyn_array_empty(readyQ) && runningProcess.burstTime == 0){
				stats.averageLatencyTime += timer;			
			}
			//error check for extract_front function; we are extracting new/first process from readyQ
			if(!dyn_array_extract_front(readyQ, &runningProcess)){
				break;
			}
		}
		//if readyQ is not empty or the running time of current process is greater than zero continue throwing current processes
		// onto virtualCPU
		if(!dyn_array_empty(readyQ) || runningProcess.burstTime > 0){
			virtual_cpu(&runningProcess);
		}
		++timer;//adding one to timer

	}
	//destroy readyQ created && complete stats
	dyn_array_destroy(readyQ);
	stats.averageLatencyTime /= ++procDone;
	stats.averageWallClockTime += timer;
	stats.averageWallClockTime /= procDone;
	return stats;

}

const ScheduleStats_t shortest_remaining_time_first(dyn_array_t* futureProcesses) {
	ScheduleStats_t stats;
	return stats;
}


const ScheduleStats_t round_robin(dyn_array_t* futureProcesses, const size_t quantum) {
	ScheduleStats_t stats;
	return stats;
} 


const bool create_suspended_processes_and_assign_pcbs(dyn_array_t* futureProcesses){
	// Error check incoming parameter
	if(futureProcesses) {
		// Get the size of the pass in dyn_array
		size_t fpSize = dyn_array_size(futureProcesses);
		// Process id for child
		pid_t pid;
		int i = 0;
		// Looping through all of elements(pcbS) in futureProcesses
		for(i = 0; i < fpSize; ++i) {
			// Spawn child process
			pid = fork();
			// Any value less than zero signals an error with fork() 
			if(pid < 0){
				fprintf(stderr, "fork() failed");
				return false;
			}
			// Zero value signals that we are the child process 
			else if(pid == 0){
				dyn_array_destroy(futureProcesses); /*destory futureProcesses...remember effect of COW*/
				while(1);
			}
			// Any value greater than zero signals that we are the parent process
			else if(pid > 0) {
				// Assign pid to PCB 
				ProcessControlBlock_t* pcb = dyn_array_at(futureProcesses, i);
				pcb->pid = pid;
				kill(pcb->pid, SIGSTOP); // Suspend child process
			}
		}
		return true;
	}	
	return false; 
}


const bool fetch_new_processes(dyn_array_t* newProcesses, dyn_array_t* futureProcesses, const size_t currentClockTime) {
	//error check incoming parameters
	if(!futureProcesses || !newProcesses){
		return false;
	}
	//create incrementer, counter of processes moved from fp to np, PCB to look and extract
	int i =0, counter = 0;
	ProcessControlBlock_t* backFp;
	ProcessControlBlock_t newPCB;
	//get size of fp and sort fp by arrival time 
	size_t fpSize = dyn_array_size(futureProcesses);
	rearranged_process_control_blocks_by_arrival_time(futureProcesses);
	//loop through fp, moving PCBs with less than or equal to currentClockTime
	for(i=0; i < fpSize; i++){
		//looking at back of dyn checking the arrival time
		backFp = dyn_array_back(futureProcesses);
		if(backFp->arrivalTime <= currentClockTime){
			//error check extract back
			if(!dyn_array_extract_back(futureProcesses, &newPCB))
				return false;
			if(dyn_array_push_front(newProcesses, &newPCB))
				++counter;//incrementer here to state that we have moved at least one PCB from fp to np
		}
		//if counter is zero == we have not moved at least one taking into account they are sorted by arrivalTime this guarantees no PCB eligible to be added
		if(counter == 0){
			return false;
		}
		//if this statement is true there are no more PCB in fp to move to np, such break out of current loop to return true meaning that we moved at least one PCB from fp
		// to np
		if (counter < i){
			break;
		}
	}
	return true;
}

void virtual_cpu(ProcessControlBlock_t* runningProcess) {

	if (runningProcess == NULL || !runningProcess->pid) {
		return;	
	}
	ssize_t err = 0;
	err = kill(runningProcess->pid,SIGCONT);
	if (err < 0) {
		fprintf(stderr,"VCPU FAILED to CONT process %d\n",runningProcess->pid);
	}
	--runningProcess->burstTime;
	err = kill(runningProcess->pid,SIGSTOP);
	if (err < 0) {
		fprintf(stderr,"VCPU FAILED to STOP process %d\n",runningProcess->pid);
	}

}

const bool load_process_control_blocks_from_file(dyn_array_t* futureProcesses, const char* binaryFileName)
{
	//error check futureProceses
	if(futureProcesses) {
		//create file descriptor for passed in binaryFile
		int fd = open(binaryFileName, O_RDONLY);
		//fd not -1 ==> binaryFileName was open correctly
		if(fd != -1) 
		{
			//used for the number of pcbS in the binaryFileName
			unsigned int numBlocks = NULL;
			//read first 32-bit unsigned representing the number of pcbS
			if(read(fd, &numBlocks, sizeof(unsigned int)))
			{	//declare a incrementor && buffer for each pcb 
				size_t i = 0;
				// create PCB_t buffer, reading each PCB && inserting them into futureProcesses, continuing with for(...) loop
				ProcessControlBlock_t data;
				for(i = 0; i < numBlocks; ++i) 
				{
					if(read(fd, &data, sizeof(ProcessControlBlock_t))) 
					{
						if(dyn_array_insert(futureProcesses, i, &data)) 
						{	
							continue;
						}
						else 
						{
							close(fd);
							return false;
						}
					}
					close(fd);
					return false;
				}
				close(fd);
				//end of looping through binaryFile adding pcbS
				return true;	
			}
			close(fd);
			//fprintf(stderr, "\nReading number of PCBs error\n");/*Throw Error : : read(fd, &numBlocks, sizeof(unsigned int)) */
			return false;
		}
		close(fd);
		//fprintf(stderr, "\nopen file error\n");
		return false;
	}
	return false;
}

const bool rearranged_process_control_blocks_by_arrival_time(dyn_array_t* futureProcesses) {
	// Error check incoming parameter
	if(!futureProcesses) {
		return false;
	}
	// Sort futureProcess by arrivalTime returning results validity 
	if(dyn_array_sort(futureProcesses, &compare))
		return true;
	else {
		fprintf(stderr,"\nsort function error\n");
		return false;
	}
}
// Used to compare arrival time of PCBs aiding sort function from greatest to least 
int compare(const void* a, const void *b) {
	const size_t aValue = ((ProcessControlBlock_t*)a)->arrivalTime;
	const size_t bValue = ((ProcessControlBlock_t*)b)->arrivalTime;
	return bValue - aValue;
}
// Used to compare burst time of PCBs aiding sort function from least to greatest
int compareBurstTime(const void* a, const void*b){
	const size_t aValue = ((ProcessControlBlock_t*) a)->burstTime;
	const size_t bValue = ((ProcessControlBlock_t*) b)->burstTime;
	return aValue - bValue;	
}
