#include "ShellPrompt.h"
#include "Kernel.h"
#include "SystemConfigReader.h"

#include <vector>
#include <sstream>
#include <cstdint>

namespace {
    const char* CSOPESY_ASCII_ART = R"(
  ___  ____   __  ____  ____  ____  _  _
 / __)/ ___) /  \(  _ \(  __)/ ___)( \/ )
( (__ \___ \(  O )) __/ ) _) \___ \ )  /
 \___)(____/ \__/(__)  (____)(____/(__/
)";
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
                kernel.print("Configuration loaded successfully. Sending to kernel...\n");
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
        } else if (command.empty()) {
            kernel.print("Please enter a command.\n");
            continue;
        }

        processCommand(command);
    }
}

void ShellPrompt::screenMenu(Process* process) {
    std::string screen_command;
    const std::string screen_prompt_str = process->getPname() + ":\\> ";

    while (true) {
        screen_command = kernel.readLine(screen_prompt_str);

        if (screen_command == "process-smi") {
            kernel.printSmi(process);
        } else if (screen_command == "exit") {
            break;
        } else {
            kernel.print("Invalid command. Please type 'process-smi' or 'exit'.\n");
        }
    }

    kernel.clearScreen();
    kernel.print(CSOPESY_ASCII_ART);
    kernel.print("\n");
    kernel.print("Hello! Welcome to the CSOPESY commandline!\n");
    kernel.print("\n");
    kernel.print("Type 'help' for available commands, or 'exit' to quit.\n");
}

void ShellPrompt::processCommand(const std::string& command) {
    if (command == "help") {
        showHelp();
    } else if (command == "echo") {
        kernel.print("Echo command received. (Arguments not yet parsed)\n");
    } else if (command == "clear") {
        kernel.clearScreen();
        kernel.print(CSOPESY_ASCII_ART);
        kernel.print("\n");
        kernel.print("Hello! Welcome to the CSOPESY commandline!\n");
        kernel.print("\n");
        kernel.print("Type 'help' for available commands, or 'exit' to quit.\n");
    } else if (command == "scheduler-start") {
        kernel.startProcessGeneration();
    } else if (command == "scheduler-stop") {
        kernel.stopProcessGeneration();
    } else if (command == "screen -ls") {
        kernel.listStatus();
    } else if (command.rfind("screen -r", 0) == 0) {
        std::istringstream iss(command); // Create a string stream from the command
        std::string cmd, subCmd, processName;
        iss >> cmd >> subCmd >> processName; // Extract "screen", "-r", and the process name

        if (!processName.empty()) { // Check if a process name was provided
            Process* processFound = nullptr;
            processFound = kernel.reattachToProcess(processName); // Call a new kernel function to reattach
            if (processFound) {
                screenMenu(processFound);
            } else {
                kernel.print("Process with name '" + processName + "' not found or terminated.\n");
            }
        } else {
            kernel.print("Usage: screen -r <process_name>\n"); // Inform the user of correct usage
        }
    } else if (command.rfind("screen -s", 0) == 0) {
        std::istringstream iss(command); // Create a string stream from the command
        std::string cmd, subCmd, memStr, processName;
        uint32_t memValue = 64;
        iss >> cmd >> subCmd >> memStr >> processName; // Extract "screen", "-s", process memory size, and the process name

        if (!memStr.empty()){
            std::istringstream tempIss(memStr);
            unsigned long temp = 0;
            if (tempIss >> temp && tempIss.eof()) {
                if (temp >= 64 && temp <= std::numeric_limits<uint32_t>::max()) {
                    memValue = static_cast<uint32_t>(temp);
                } else {
                    kernel.print("Error: Memory size must be a positive integer within the valid range.\n");
                    return;
                }
            } else {
                kernel.print("Error: Invalid memory size format. Please enter a positive whole number.\n");
                return;
            }
        }

        if (!processName.empty()) {
            Process* processFound = nullptr;
            processFound = kernel.reattachToProcess(processName); // Call a new kernel function to reattach
            if (processFound) {
                screenMenu(processFound);
            } else {
                processFound = kernel.startProcess(processName, memValue);
                screenMenu(processFound);
            }
        } else {
            kernel.print("Usage: screen -s <process_memory_size> <process_name>\n"); // Inform the user of correct usage
        }
    } else if (command == "process-smi") {
        kernel.printMemoryUtilizationReport();
    } else if (command == "vmstat") {
        kernel.printMemoryStatistics();
    } else {
        kernel.print("Unknown command: '" + command + "'. Type 'help' for assistance.\n");
    }
}

void ShellPrompt::showHelp() const {
    kernel.print("\n--- Available Commands ---\n");
    kernel.print("exit                                              - Quits the main OS shell.\n");
    kernel.print("help                                              - Displays this help message.\n");
    kernel.print("echo                                              - A simple placeholder command.\n");
    kernel.print("clear                                             - Clears the terminal screen.\n");
    kernel.print("scheduler-start                                   - Starts automatic process generation.\n");
    kernel.print("scheduler-stop                                    - Stops automatic process generation.\n");
    kernel.print("screen -ls                                        - Lists CPU utilization, core usage, and a summary of all running and finished processes\n");
    kernel.print("screen -r <process_name>                          - Reattach to the screen of an existing process\n");
    kernel.print("screen -s <process_memory_size> <process_name>    - Start a new process\n");
    kernel.print("process-smi                                       - Print a summarized view of the memory allocation and CPU utilization\n");
    kernel.print("vmstat                                            - Print a detailed view of the memory allocation\n");
    kernel.print("--------------------------\n\n");
}
