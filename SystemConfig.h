#pragma once

#include <string>
#include <cstdint>

enum class SchedulerType {
    ROUND_ROBIN, // "rr"
    FCFS         // "fcfs"
    // Add other types as needed
};

/**
 * @struct SystemConfig
 * @brief Holds all configuration parameters for the OS.
 */
struct SystemConfig {
    uint32_t numCPUs = 1;                                   // Default to 1 CPU
    SchedulerType scheduler = SchedulerType::ROUND_ROBIN;   // Default to Round Robin
    uint32_t quantumCycles = 5;                             // Default quantum
    uint32_t batchProcessFreq = 1;                          // Default batch process frequency
    uint32_t minInstructions = 1;                           // Default min instructions per process
    uint32_t maxInstructions = 10;                          // Default max instructions per process
    uint32_t delaysPerExec = 0;                             // Default delay per instruction execution
    uint32_t maxOverallMem = 64;                            // Default max overall memory (e.g., 64KB)
    uint32_t memPerFrame = 16;                              // Default memory per frame (e.g., 16KB)
    uint32_t memPerProc = 64;                               // Default memory per process (e.g., 64KB)
};
