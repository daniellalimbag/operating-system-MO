#include "Config.h"
#include <fstream>
#include <iostream>
#include <cstdint>

bool readConfigFromFile(const std::string& filename, SystemConfig& config) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open config.txt\n";
        return false;
    }

    std::string key;
    while (file >> key) {
        if (key == "num-cpu") {
            file >> config.numCPU;
        }
        else if (key == "scheduler") {
            std::string schedulerValue;
            file >> schedulerValue;
            if (schedulerValue.front() == '"' && schedulerValue.back() == '"') {
                schedulerValue = schedulerValue.substr(1, schedulerValue.length() - 2);
            }
            config.scheduler = schedulerValue;
        }
        else if (key == "quantum-cycles") {
            uint32_t val;
            file >> val;
            config.quantumCycles = val;
        }
        else if (key == "batch-process-freq") {
            uint32_t val;
            file >> val;
            config.batchProcessFreq = val;
        }
        else if (key == "min-ins") {
            uint32_t val;
            file >> val;
            config.minInstructions = val;
        }
        else if (key == "max-ins") {
            uint32_t val;
            file >> val;
            config.maxInstructions = val;
        }
        else if (key == "delay-per-exec") {
            uint32_t val;
            file >> val;
            config.delaysPerExec = val;
        }
        else if (key == "max-overall-mem") {
            uint32_t val;
            file >> val;
            if (val < 64 || val > 65536 || (val & (val - 1)) != 0) {
                std::cerr << "Error: max-overall-mem must be a power of 2 in [2^6, 2^16] (64 to 65536)\n";
                return false;
            }
            config.maxOverallMem = val;
        }
        else if (key == "mem-per-frame") {
            uint32_t val;
            file >> val;
            //Adjusted range for the purposes of this HW
            if (val < 16 || val > 65536 || (val & (val - 1)) != 0) {
                std::cerr << "Error: mem-per-frame must be a power of 2 in [2^4, 2^16] (64 to 65536)\n";
                return false;
            }
            config.memPerFrame = val;
        }
        //NOTE: For MO2, this should be changed to min-mem-per-proc and max-mem-per-proc
        else if (key == "mem-per-proc") {
            uint32_t val;
            file >> val;
            if (val < 64 || val > 65536 || (val & (val - 1)) != 0) {
                std::cerr << "Error: mem-per-proc must be a power of 2 in [2^6, 2^16] (64 to 65536)\n";
                return false;
            }
            config.memPerProc = val;
        }
        else {
            std::cerr << "Warning: Unknown config parameter: " << key << "\n";
            std::string dummy;
            file >> dummy;
        }
    }

    file.close();
    return true;
}