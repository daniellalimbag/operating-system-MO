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
    uint32_t numCPUs = 4U;                                  // Default to 4 CPU
    SchedulerType scheduler = SchedulerType::ROUND_ROBIN;   // Default to Round Robin
    uint32_t quantumCycles = 5U;                            // Default quantum
    uint32_t batchProcessFreq = 1U;                         // Default batch process frequency
    uint32_t minInstructions = 1000U;                       // Default min instructions per process
    uint32_t maxInstructions = 2000U;                       // Default max instructions per process
    uint32_t delaysPerExec = 0U;                            // Default delay per instruction execution
    uint32_t maxOverallMem = 64U;                           // Default max overall memory (e.g., 64KB)
    uint32_t memPerFrame = 16U;                             // Default memory per frame (e.g., 16KB)
    uint32_t memPerProc = 64U;                              // Default memory per process (e.g., 64KB)
};
