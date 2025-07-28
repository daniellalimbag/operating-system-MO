#pragma once

#include "SystemConfig.h"
#include "Kernel.h"

#include <limits>
#include <fstream>

bool isPowerOfTwo(uint32_t n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

bool readConfigFromFile(const std::string& filename, SystemConfig& config, Kernel& kernel) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        kernel.print("Error: Could not open config.txt. Using default kernel parameters.\n");
        return false;
    }

    kernel.print("Reading configuration from '" + filename + "'...\n");

    std::string key;
    bool success = true; // Track overall success of parsing
    while (file >> key) {
        if (key == "num-cpu") {
            uint32_t val;
            if (file >> val) {
                config.numCPUs = val;
            } else {
                kernel.print("Warning: Malformed value for num-cpu. Using default.\n"); success = false;
                file.clear(); file.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear error and ignore rest of line
            }
        }
        else if (key == "scheduler") {
            std::string schedulerValue;
            if (file >> schedulerValue) {
                // Remove quotes if present
                if (schedulerValue.front() == '"' && schedulerValue.back() == '"') {
                    schedulerValue = schedulerValue.substr(1, schedulerValue.length() - 2);
                }
                if (schedulerValue == "rr") {
                    config.scheduler = SchedulerType::ROUND_ROBIN;
                } else if (schedulerValue == "fcfs") {
                    config.scheduler = SchedulerType::FCFS;
                } else {
                    kernel.print("Warning: Unknown scheduler value '" + schedulerValue + "'. Using default (Round Robin).\n"); success = false;
                }
            } else {
                kernel.print("Warning: Malformed value for scheduler. Using default.\n"); success = false;
                file.clear(); file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
        else if (key == "quantum-cycles") {
            uint32_t val;
            if (file >> val) { config.quantumCycles = val; } else {
                kernel.print("Warning: Malformed value for quantum-cycles. Using default.\n"); success = false;
                file.clear(); file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
        else if (key == "batch-process-freq") {
            uint32_t val;
            if (file >> val) { config.batchProcessFreq = val; } else {
                kernel.print("Warning: Malformed value for batch-process-freq. Using default.\n"); success = false;
                file.clear(); file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
        else if (key == "min-ins") {
            uint32_t val;
            if (file >> val) { config.minInstructions = val; } else {
                kernel.print("Warning: Malformed value for min-ins. Using default.\n"); success = false;
                file.clear(); file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
        else if (key == "max-ins") {
            uint32_t val;
            if (file >> val) { config.maxInstructions = val; } else {
                kernel.print("Warning: Malformed value for max-ins. Using default.\n"); success = false;
                file.clear(); file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
        else if (key == "delay-per-exec") {
            uint32_t val;
            if (file >> val) { config.delaysPerExec = val; } else {
                kernel.print("Warning: Malformed value for delay-per-exec. Using default.\n"); success = false;
                file.clear(); file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
        else if (key == "max-overall-mem") {
            uint32_t val;
            if (file >> val) {
                if (val < 64 || val > 65536 || !isPowerOfTwo(val)) {
                    kernel.print("Error: max-overall-mem must be a power of 2 in [64, 65536]. Using default.\n");
                    success = false;
                } else {
                    config.maxOverallMem = val;
                }
            } else {
                kernel.print("Warning: Malformed value for max-overall-mem. Using default.\n"); success = false;
                file.clear(); file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
        else if (key == "mem-per-frame") {
            uint32_t val;
            if (file >> val) {
                if (val < 64 || val > 65536 || !isPowerOfTwo(val)) {
                    kernel.print("Error: mem-per-frame must be a power of 2 in [64, 65536]. Using default.\n");
                    success = false;
                } else {
                    config.memPerFrame = val;
                }
            } else {
                kernel.print("Warning: Malformed value for mem-per-frame. Using default.\n"); success = false;
                file.clear(); file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
        else if (key == "min-mem-per-proc") {
            uint32_t val;
            if (file >> val) {
                if (val < 64 || val > 65536 || !isPowerOfTwo(val)) {
                    kernel.print("Error: min-mem-per-proc must be a power of 2 in [64, 65536]. Using default.\n");
                    success = false;
                } else {
                    config.minMemPerProc = val;
                }
            } else {
                kernel.print("Warning: Malformed value for mem-per-proc. Using default.\n"); success = false;
                file.clear(); file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
        else if (key == "max-mem-per-proc") {
            uint32_t val;
            if (file >> val) {
                if (val < 64 || val > 65536 || !isPowerOfTwo(val)) {
                    kernel.print("Error: max-mem-per-proc must be a power of 2 in [64, 65536]. Using default.\n");
                    success = false;
                } else {
                    config.maxMemPerProc = val;
                }
            } else {
                kernel.print("Warning: Malformed value for mem-per-proc. Using default.\n"); success = false;
                file.clear(); file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
        }
        else {
            kernel.print("Warning: Unknown config parameter: " + key + "\n");
            std::string dummy;
            file >> dummy; // Read and discard the value of the unknown key
        }
    }

    file.close();
    return success;
}
