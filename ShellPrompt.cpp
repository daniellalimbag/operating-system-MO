#include "ShellPrompt.h"
#include "Kernel.h"
#include "SystemConfigReader.h"

#include <vector>
#include <sstream>
#include <cstdint>

// ===================================================
// Anonymous helper namespace
// ===================================================

namespace {
    const char* CSOPESY_ASCII_ART = R"(
  ___  ____   __  ____  ____  ____  _  _
 / __)/ ___) /  \(  _ \(  __)/ ___)( \/ )
( (__ \___ \(  O )) __/ ) _) \___ \ )  /
 \___)(____/ \__/(__)  (____)(____/(__/
)";
    const std::string INITIAL_PROMPT_STR = "OS_Boot>";
    const std::string MAIN_SHELL_PROMPT_STR = "root:\\>";
    const std::string INITIAL_BOOT_INSTRUCTIONS = "To start the main shell, type 'initialize'. To shut down, type 'exit'.";
    const std::string MAIN_SHELL_INSTRUCTIONS = "Type 'help' for available commands, or 'exit' to quit.";
}
struct ParsedCommand {
    std::string commandName;
    std::vector<std::string> args;
};

// ===================================================
// Public functions
// ===================================================

ShellPrompt::ShellPrompt(Kernel& kernel)
    : kernel(kernel)
{
    setupCommands();
}

void ShellPrompt::run() {
    kernel.clearScreen();
    showHeader(INITIAL_BOOT_INSTRUCTIONS);

    if (!runInitialBootPrompt(INITIAL_PROMPT_STR)) {
        return;
    }

    showHeader(MAIN_SHELL_INSTRUCTIONS);
    runMainShellLoop(MAIN_SHELL_PROMPT_STR);
}

// ===================================================
// Private helper functions
// ===================================================

void ShellPrompt::showHeader(const std::string& instructions) const {
    kernel.print(CSOPESY_ASCII_ART);
    kernel.print("\n");
    kernel.print("Hello! Welcome to the CSOPESY commandline!\n");
    kernel.print("\n");
    kernel.print(instructions);
    kernel.print("\n");
}

bool ShellPrompt::runInitialBootPrompt(const std::string& prompt) {
    std::string command;
    while (true) {
        command = kernel.readLine(prompt);

        if (command == "initialize") {
            initializeKernel();
            return true;
        } else if (command == "exit") {
            return false;
        } else {
            kernel.print("Invalid command. Please type 'initialize' or 'exit'.\n");
        }
    }
}

void ShellPrompt::initializeKernel() {
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
}

void ShellPrompt::runMainShellLoop(const std::string& prompt) {
    std::string command;
    while (true) {
        command = kernel.readLine(prompt);

        ParsedCommand parsed = parseCommand(command);
        if (!executeCommand(parsed)) {
            break;
        }
    }
}

ParsedCommand ShellPrompt::parseCommand(const std::string& command) {
    ParsedCommand parsed;
    std::istringstream iss(command);

    // First word is the command name
    iss >> parsed.commandName;

    // The rest are args
    std::string arg;
    while (iss >> arg) {
        parsed.args.push_back(arg);
    }

    return parsed;
}

bool ShellPrompt::executeCommand(const ParsedCommand& parsed) {
    auto it = commandHandlers.find(parsed.commandName);
    if (it != commandHandlers.end()) {
        return it->second(parsed.args);
    } else {
        kernel.print("Unknown command: '" + parsed.commandName + "'. Type 'help' for assistance.\n");
        return true;
    }
}

void ShellPrompt::showHelp() const {
    kernel.print("\n--- Available Commands ---\n");
    kernel.print("exit                                              - Quits the main OS shell.\n");
    kernel.print("help                                              - Displays this help message.\n");
    kernel.print("echo                                              - Echoes the command back.\n");
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

void ShellPrompt::setupCommands() {
    commandHandlers["exit"] = [this](const std::vector<std::string>& args) -> bool {
        if (!args.empty()) {
            kernel.print("Usage: exit\n");
            return true;
        }
        return false;
    };
    commandHandlers["help"] = [this](const std::vector<std::string>& args) -> bool {
        if (!args.empty()) {
            kernel.print("Usage: help\n");
            return true;
        }
        showHelp();
        return true;
    };
    commandHandlers["echo"] = [this](const std::vector<std::string>& args) -> bool {
        if (!args.empty()) {
            kernel.print("Usage: echo <message>\n");
            return true;
        }
        kernel.print("echo");
        for (const auto& arg : args) {
            kernel.print(" ");
            kernel.print(arg);
        }
        kernel.print("\n");
        return true;
    };
    commandHandlers["clear"] = [this](const std::vector<std::string>& args) -> bool {
        if (!args.empty()) {
            kernel.print("Usage: clear\n");
            return true;
        }
        kernel.clearScreen();
        showHeader(MAIN_SHELL_INSTRUCTIONS);
        return true;
    };
    commandHandlers["scheduler-start"] = [this](const std::vector<std::string>& args) -> bool {
        if (!args.empty()) {
            kernel.print("Usage: scheduler-start\n");
            return true;
        }
        kernel.startProcessGeneration();
        return true;
    };
    commandHandlers["scheduler-stop"] = [this](const std::vector<std::string>& args) -> bool {
        if (!args.empty()) {
            kernel.print("Usage: scheduler-stop\n");
            return true;
        }
        kernel.stopProcessGeneration();
        return true;
    };
    commandHandlers["process-smi"] = [this](const std::vector<std::string>& args) -> bool {
        if (!args.empty()) {
            kernel.print("Usage: process-smi\n");
            return true;
        }
        kernel.printMemoryUtilizationReport();
        return true;
    };
    commandHandlers["vmstat"] = [this](const std::vector<std::string>& args) -> bool {
        if (!args.empty()) {
            kernel.print("Usage: vmstat\n");
            return true;
        }
        kernel.printMemoryStatistics();
        return true;
    };
    commandHandlers["screen"] = [this](const std::vector<std::string>& args) -> bool {
        if (args.empty()) {
            kernel.print("Usage: screen <subcommand> [args...]\n");
            kernel.print("Subcommands: -ls, -r, -s\n");
            return true;
        }
        const std::string& subCommand = args[0];

        if (subCommand == "-ls") {
            if (args.size() != 1) {
                kernel.print("Usage: screen -ls\n");
            } else {
                kernel.listStatus();
            }
        } else if (subCommand == "-r") {
            if (args.size() != 2) {
                kernel.print("Usage: screen -r <process_name>\n");
            } else {
                handleScreenReattach(args);
            }
        } else if (subCommand == "-s") {
            if (args.size() != 3) {
                kernel.print("Usage: screen -s <process_memory_size> <process_name>\n");
            } else {
                handleScreenStart(args);
            }
        } else {
            kernel.print("Unknown 'screen' subcommand: '" + subCommand + "'.\n");
            kernel.print("Usage: screen <subcommand> [args...]\n");
            kernel.print("Subcommands: -ls, -r, -s\n");
        }
        return true;
    };
}

void ShellPrompt::handleScreenReattach(const std::vector<std::string>& args) {
    std::string process_name = args[1];
    Process* processFound = kernel.reattachToProcess(process_name);
    if (processFound) {
        handleScreenMenu(processFound);
    } else {
        kernel.print("Process with name '" + process_name + "' not found or terminated.\n");
    }
}

void ShellPrompt::handleScreenStart(const std::vector<std::string>& args) {
    std::string memory_str = args[1];
    std::string process_name = args[2];
    uint32_t memValue = 64;

    std::istringstream iss(memory_str);
    unsigned long temp = 0;
    if (iss >> temp && iss.eof()) {
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

    Process* processFound = kernel.reattachToProcess(process_name);
    if (processFound) {
        handleScreenMenu(processFound);
    } else {
        processFound = kernel.startProcess(process_name, memValue);
        handleScreenMenu(processFound);
    }
}

void ShellPrompt::handleScreenMenu(Process* process) {
    std::string screen_command;
    const std::string screen_prompt_str = process->getPname() + ":\\>";

    while (true) {
        screen_command = kernel.readLine(screen_prompt_str);

        if (screen_command == "exit") {
            break;
        } else if (screen_command == "process-smi") {
            kernel.printSmi(process);
        } else {
            kernel.print("Invalid command. Please type 'process-smi' or 'exit'.\n");
        }
    }
    kernel.clearScreen();
    showHeader(MAIN_SHELL_INSTRUCTIONS);
}
