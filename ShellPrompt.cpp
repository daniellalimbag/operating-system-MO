#include "ShellPrompt.h"
#include "Kernel.h"
#include "SystemConfig.h"

#include <vector>
#include <sstream>
#include <fstream>
#include <cstdint>

namespace {
    const char* CSOPESY_ASCII_ART = R"(
  ___  ____   __  ____  ____  ____  _  _
 / __)/ ___) /  \(  _ \(  __)/ ___)( \/ )
( (__ \___ \(  O )) __/ ) _) \___ \ )  /
 \___)(____/ \__/(__)  (____)(____/(__/
)";

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
                    if (val < 16 || val > 65536 || !isPowerOfTwo(val)) {
                        kernel.print("Error: mem-per-frame must be a power of 2 in [16, 65536]. Using default.\n");
                        success = false;
                    } else {
                        config.memPerFrame = val;
                    }
                } else {
                    kernel.print("Warning: Malformed value for mem-per-frame. Using default.\n"); success = false;
                    file.clear(); file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                }
            }
            else if (key == "mem-per-proc") {
                uint32_t val;
                if (file >> val) {
                    if (val < 64 || val > 65536 || !isPowerOfTwo(val)) {
                        kernel.print("Error: mem-per-proc must be a power of 2 in [64, 65536]. Using default.\n");
                        success = false;
                    } else {
                        config.memPerProc = val;
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
}

ShellPrompt::ShellPrompt(Kernel& kernel)
    : kernel(kernel){}

void ShellPrompt::start() {
    kernel.clearScreen();
    kernel.print(CSOPESY_ASCII_ART);
    kernel.print("\n");
    kernel.print("Hello! Welcome to the CSOPESY commandline!\n");
    kernel.print("\n");
    kernel.print("To start the main shell, type 'initialize'. To shut down, type 'exit'.\n");

    std::string initial_command;
    const std::string initial_prompt_str = "OS_Boot> ";

    // --- Initial Boot Prompt Loop ---
    while (true) {
        initial_command = kernel.readLine(initial_prompt_str);

        if (initial_command == "initialize") {
            kernel.print("Initializing main OS shell...\n");
            SystemConfig loadedConfig;
            bool configLoaded = readConfigFromFile("config.txt", loadedConfig, kernel);
            if (configLoaded) {
                kernel.print("Configuration loaded successfully.\n");
                kernel.initialize(loadedConfig);
            } else {
                kernel.print("Configuration loading failed or had errors. Using default values for unconfigured/invalid parameters.\n");
            }

            kernel.print("Main shell active. Type 'help' for available commands, or 'exit' to quit.\n");
            break;
        } else if (initial_command == "exit") {
            kernel.print("OS shutdown initiated from boot prompt.\n");
            return;
        } else {
            kernel.print("Invalid command. Please type 'initialize' or 'exit'.\n");
        }
    }

    std::string command;
    const std::string main_shell_prompt_str = "root:\\> ";

    // --- Main Shell Loop ---
    while (true) {
        command = kernel.readLine(main_shell_prompt_str);

        if (command == "exit") {
            kernel.print("Shutting down main shell...\n");
            return;
        }

        if (command.empty()) {
            kernel.print("Please enter a command.\n");
            continue;
        }

        processCommand(command);
    }
}

void ShellPrompt::processCommand(const std::string& command) {
    if (command == "help") {
        showHelp();
    } else if (command == "echo") {
        kernel.print("Echo command received. (Arguments not yet parsed)\n");
    } else if (command == "clear") {
        kernel.clearScreen();
    }
    // else if (command == "ps") { /* Call kernel.getAllProcessesStatus() */ }
    // else if (command == "mem_usage") { /* Call kernel.getMemoryUtilizationReport() */ }
    else {
        kernel.print("Unknown command: '" + command + "'. Type 'help' for assistance.\n");
    }
}

void ShellPrompt::showHelp() const {
    kernel.print("\n--- Available Commands ---\n");
    kernel.print("exit     - Quits the main OS shell.\n");
    kernel.print("help     - Displays this help message.\n");
    kernel.print("echo     - A simple placeholder command.\n");
    kernel.print("clear    - Clears the terminal screen.\n");
    kernel.print("--------------------------\n\n");
}
