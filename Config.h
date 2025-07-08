#pragma once
#include <string>
#include <cstdint>

struct SystemConfig {
    int numCPU = 1;
    std::string scheduler = "fcfs";
    uint32_t quantumCycles = 1;
    uint32_t batchProcessFreq = 1;
    uint32_t minInstructions = 1;
    uint32_t maxInstructions = 1;
    uint32_t delaysPerExec = 0;
    uint32_t maxOverallMem = 0;
    uint32_t memPerFrame = 0;
    uint32_t memPerProc = 0;
};

bool readConfigFromFile(const std::string& filename, SystemConfig& config);
