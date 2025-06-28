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
        else {
            std::cerr << "Warning: Unknown config parameter: " << key << "\n";
            std::string dummy;
            file >> dummy;
        }
    }

    file.close();
    return true;
}