#pragma once

#include <string>
#include <cstdint>

enum class SchedulerType {
    ROUND_ROBIN, // "rr"
    FCFS         // "fcfs"
};

/**
 * @struct SystemConfig
 * @brief Holds all configuration parameters for the OS.
 */
struct SystemConfig {
    uint32_t numCPUs = 4U;                                  // Default to 4 CPU
    SchedulerType scheduler = SchedulerType::ROUND_ROBIN;   // Default to Round Robin
    uint32_t quantumCycles = 5U;                            // Default quantum
    uint32_t batchProcessFreq = 1U;                         // Default batch process frequency
    uint32_t minInstructions = 10U;                       // Default min instructions per process
    uint32_t maxInstructions = 20U;                       // Default max instructions per process
    uint32_t delaysPerExec = 0U;                            // Default delay per instruction execution
    uint32_t maxOverallMem = 256U;                          // Default max overall memory (e.g., 256B)
    uint32_t memPerFrame = 128U;                            // Default memory per frame (e.g., 128B)
    uint32_t minMemPerProc = 64U;                           // Default min memory per process (e.g., 64B)
    uint32_t maxMemPerProc = 128U;                          // Default max memory per process (e.g., 128B)
};
