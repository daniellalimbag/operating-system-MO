#pragma once
#include <string>

struct SystemConfig {
    int numCPU = 1;
    std::string scheduler = "fcfs";
    int quantumCycles = 1;
    int batchProcessFreq = 1;
    int minInstructions = 1;
    int maxInstructions = 1;
    int delaysPerExec = 0;
};
