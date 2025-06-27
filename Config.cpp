#include "Config.h"
#include <fstream>
#include <iostream>

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
            file >> config.quantumCycles;
        }
        else if (key == "batch-process-freq") {
            file >> config.batchProcessFreq;
        }
        else if (key == "min-ins") {
            file >> config.minInstructions;
        }
        else if (key == "max-ins") {
            file >> config.maxInstructions;
        }
        else if (key == "delay-per-exec") {
            file >> config.delaysPerExec;
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