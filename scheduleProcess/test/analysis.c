#include <stdio.h>
#include <stdlib.h>
#include <dyn_array.h>
#include <string.h>


#include "../include/process_scheduling.h"

int main(int argc, char*argv[]) {

	if (argc < 3) {
		fprintf(stdout,"Program needs at least 3 parameters to run\n");
		return -1;
	}
	char* pEnd = NULL;
	const size_t scheduleAlgorithm = strtol(argv[2],&pEnd,10);
	if (scheduleAlgorithm < 1 || scheduleAlgorithm > 4) {
		printf("SCHEDULE ALGORITHMS:\n\n \
				1)FIRST COME FIRST SERVED\n \
				2)SHORTEST JOB FIRST\n \
				3)SHORTEST REMAINING TIME FIRST\n \
				4)ROUND ROBIN\n");
		return -1;
	}
	//made up size, just creating a random size can expanded so whatever
	dyn_array_t* futureProcesses = dyn_array_create(16,sizeof(ProcessControlBlock_t),NULL);
	load_process_control_blocks_from_file(futureProcesses,argv[1]);	
	create_suspended_processes_and_assign_pcbs(futureProcesses);
	rearranged_process_control_blocks_by_arrival_time(futureProcesses);
	
	ScheduleStats_t stats;

	switch(scheduleAlgorithm) {
		case 1:
			stats = first_come_first_served(futureProcesses); 
		break;
		case 2:
			stats = shortest_job_first(futureProcesses);
		break;
		case 3:
			stats = shortest_remaining_time_first(futureProcesses);
		break;
		case 4:
			stats = round_robin(futureProcesses,5);
		break;
	}
	printf("AVG WALL CLOCK TIME = %f\nAVG LATENCY TIME = %f\n", stats.averageWallClockTime, stats.averageLatencyTime);
	dyn_array_destroy(futureProcesses);
	return 0;

}
