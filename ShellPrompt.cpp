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
    } else if (command == "scheduler-start") {
        kernel.startProcessGeneration();
    } else if (command == "scheduler-stop") {
        kernel.stopProcessGeneration();
    } else if (command == "screen -ls") {
        kernel.listStatus();
    }
    // else if (command == "screen -ls") { /* Call kernel.getAllProcessesStatus() */ }
    // else if (command == "process-smi") { /* Call kernel.getMemoryUtilizationReport() */ }
    else {
        kernel.print("Unknown command: '" + command + "'. Type 'help' for assistance.\n");
    }
}

void ShellPrompt::showHelp() const {
    kernel.print("\n--- Available Commands ---\n");
    kernel.print("exit             - Quits the main OS shell.\n");
    kernel.print("help             - Displays this help message.\n");
    kernel.print("echo             - A simple placeholder command.\n");
    kernel.print("clear            - Clears the terminal screen.\n");
    kernel.print("scheduler-start  - Starts automatic process generation.\n");
    kernel.print("scheduler-stop   - Stops automatic process generation.\n");
    kernel.print("screen -ls       - Lists CPU utilization, core usage, and a summary of all running and finished processes");
    kernel.print("--------------------------\n\n");
}
