#include<iostream>
#include<vector>
#include<random>
#include<fstream>
#include <cstring>
#include <cstdio>
#include <unistd.h>



#define DISPLAY 1

struct ProcessControlBlock{
	uint32_t burstTime; //CPU runtime for Process
	uint32_t arrivalTime; // The time the process comes into the scheduler
	int32_t pid; // Used to bind a running process with PCB
	uint32_t activated;

};

int main (int argc, char** argv) {

	if (argc < 4) {
		std::cout << "Invalid usage is: " << argv[0] << " <binary file name> <number of process control blocks> <random seed value>" << std::endl;
		return -1;
	}
	
	std::string binaryFile(argv[1]);
	size_t numsOfPCBs = atoi(argv[2]);
	unsigned int seedValue = atoi(argv[3]);

	std::default_random_engine gen(seedValue);
	std::normal_distribution<double> normalDistribution(15.0,4.0);

	std::uniform_int_distribution<size_t> uniformDistribution(1,30);
	
	size_t i = 0;

	std::vector<ProcessControlBlock> pcb(numsOfPCBs);
	memset(pcb.data(),0,sizeof(ProcessControlBlock) * numsOfPCBs);

	for (; i < numsOfPCBs; ++i) {
			
		if ((i % 2) == 0) {
			pcb[i].arrivalTime = normalDistribution(gen);
			pcb[i].burstTime = uniformDistribution(gen);
		}
		else {
			pcb[i].burstTime = normalDistribution(gen);
			pcb[i].arrivalTime = uniformDistribution(gen);
		}
	}

#if DISPLAY
	for (size_t i = 0; i < pcb.size(); ++i) {
		std::cout << pcb[i].burstTime << " "
				  << pcb[i].arrivalTime << " "
				  << pcb[i].pid << " "
				  << pcb[i].activated << " "
				  << std::endl;
	}
#endif


	i = 0;
	std::ofstream outFile;
	outFile.open(binaryFile.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);

	size_t bytesToWrite = (sizeof(ProcessControlBlock) * pcb.size()) + sizeof(uint32_t);
	char* buffer = new char[bytesToWrite];
	memcpy(buffer,&numsOfPCBs,sizeof(uint32_t));
	memcpy(buffer + sizeof(uint32_t),pcb.data(),sizeof(ProcessControlBlock) * pcb.size());
	outFile.write(buffer,bytesToWrite);
	outFile.close();

	return 0;

}
