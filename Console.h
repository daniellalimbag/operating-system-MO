#ifndef CONSOLE_H
#define CONSOLE_H

#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <ctime>
#ifdef _WIN32
#include <conio.h>
#endif
#include "Header.h"
#include "Process.h"
#include "ProcessManager.h"
#include "Scheduler.h"
#include "Config.h"
#include <fstream>

class OpesyConsole {
private:
    ProcessManager processManager;
    Scheduler scheduler{processManager};
    SystemConfig config;
    bool initialized = false;
    std::thread processGeneratorThread;
    std::atomic<bool> generating = false;
    int processCounter = 1;

    void processGenerationLoop() {
        while (generating) {
            std::ostringstream oss;
            oss << "p" << std::setw(2) << std::setfill('0') << processCounter++;
            std::string name = oss.str();
            int pid = processManager.createProcess(name);
            Process* proc = processManager.getProcess(pid);
            if (proc) {
                int instructionCount = rand() % (config.maxInstructions - config.minInstructions + 1) + config.minInstructions;
                std::vector<std::string> declaredVars;
                int numVars = std::max(1, rand() % 3 + 1); // 1-3 variables
                // Declare variables
                for (int v = 0; v < numVars; ++v) {
                    std::string var = "v" + std::to_string(v+1);
                    proc->addInstruction("DECLARE " + var);
                    declaredVars.push_back(var);
                }
                int i = 0;
                while (i < instructionCount) {
                    int choice = rand() % 6; // 0:ADD, 1:SUB, 2:SLEEP, 3:PRINT, 4:FOR, 5:PRINT default
                    if (choice == 0 && !declaredVars.empty()) { // ADD
                        std::string var = declaredVars[rand() % declaredVars.size()];
                        std::string rhs = (rand() % 2 == 0 && !declaredVars.empty()) ? declaredVars[rand() % declaredVars.size()] : std::to_string(rand() % 100);
                        proc->addInstruction("ADD " + var + " " + rhs);
                        ++i;
                    } else if (choice == 1 && !declaredVars.empty()) { // SUBTRACT
                        std::string var = declaredVars[rand() % declaredVars.size()];
                        std::string rhs = (rand() % 2 == 0 && !declaredVars.empty()) ? declaredVars[rand() % declaredVars.size()] : std::to_string(rand() % 100);
                        proc->addInstruction("SUBTRACT " + var + " " + rhs);
                        ++i;
                    } else if (choice == 2) { // SLEEP
                        int ticks = rand() % 5 + 1;
                        proc->addInstruction("SLEEP " + std::to_string(ticks));
                        ++i;
                    } else if (choice == 3) { // PRINT with message
                        proc->addInstruction("PRINT(\"Hello from " + name + " - line " + std::to_string(i + 1) + "\")");
                        ++i;
                    } else if (choice == 4 && i + 4 < instructionCount && !declaredVars.empty()) { // FOR loop (with at least 2 body instructions)
                        std::string loopVar = "loop" + std::to_string(rand() % 100);
                        int start = rand() % 3;
                        int end = start + rand() % 3 + 1; // 1-3 iterations
                        proc->addInstruction("FOR " + loopVar + " " + std::to_string(start) + " " + std::to_string(end));
                        // Add 2-3 body instructions
                        int bodyCount = 2 + rand() % 2;
                        for (int b = 0; b < bodyCount; ++b) {
                            int bodyChoice = rand() % 3;
                            if (bodyChoice == 0 && !declaredVars.empty()) {
                                std::string var = declaredVars[rand() % declaredVars.size()];
                                proc->addInstruction("ADD " + var + " " + std::to_string(rand() % 10));
                            } else if (bodyChoice == 1 && !declaredVars.empty()) {
                                std::string var = declaredVars[rand() % declaredVars.size()];
                                proc->addInstruction("SUBTRACT " + var + " " + std::to_string(rand() % 10));
                            } else {
                                proc->addInstruction("PRINT");
                            }
                            ++i;
                        }
                        proc->addInstruction("END FOR");
                        i += 2; // FOR + END FOR
                    } else { // PRINT default
                        proc->addInstruction("PRINT");
                        ++i;
                    }
                }
                proc->addInstruction("EXIT");
            }
            scheduler.addProcess(pid);
            int delay = config.batchProcessFreq;
            for (int i = 0; i < delay && generating; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    void clearScreen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }

    void displayProcessInfo(const std::string& sessionName, int pid, bool showDetails = false) {
        auto process = processManager.getProcess(pid);
        if (!process) return;

        std::cout << "Process name: " << sessionName << std::endl;

        if (showDetails) {
            std::cout << "ID: " << pid << std::endl;
            std::cout << "Core: " << (process->getCore() == -1 ? "N/A" : std::to_string(process->getCore())) << std::endl;
            std::cout << "CPU Utilization: " << process->getCPUUtilization() << "%" << std::endl;
        }

        std::cout << "Logs:" << std::endl << std::endl;

        std::string logs = process->getLogs();
        std::cout << (logs.empty() ? "No logs available yet." : logs) << std::endl;

        if (process->isComplete()) {
            std::cout << "Finished!" << std::endl;
        } else {
            std::cout << "Current instruction line: " << process->getCurrentInstructionNumber()
                     << " / " << process->getTotalInstructions() << std::endl;
        }
        std::cout << std::endl;
    }

    bool handleScreenCommand(const std::string& command) {
        if (command.rfind("screen -s ", 0) == 0) {
            std::string processName = command.substr(10);
            int pid = processManager.createProcess(processName);
            std::cout << "Created screen session '" << processName << "'" << std::endl;
            scheduler.addProcess(pid);
            sessionLoop(processName, pid);
            return true;
        }

        if (command.rfind("screen -r ", 0) == 0) {
            std::string processName = command.substr(10);
            auto processes = processManager.getAllProcesses();

            for (const auto& process : processes) {
                if (process->getProcessName() == processName) {
                    sessionLoop(processName, process->getPID());
                    return true;
                }
            }
            std::cout << "Screen session '" << processName << "' does not exist." << std::endl;
            return true;
        }

        return false;
    }

    void displayProcessesByStatus(bool showRunning) {
        auto processes = processManager.getAllProcesses();
        std::cout << (showRunning ? "Running" : "Finished") << " processes:" << std::endl;

        for (const auto& process : processes) {
            bool isRunning = !process->isComplete();
            int core = process->getCore();
            if (isRunning == showRunning) {
                if (showRunning && core == -1) continue; // To be removed? (Only displays processes that have been assigned a core)
                auto timestamp = process->getTimestamp();
                std::cout << process->getProcessName() << "\t(" << timestamp << ")\t";

                size_t currentLine = process->getCurrentInstructionNumber();
                size_t totalLines = process->getTotalInstructions();

                if (showRunning) {
                    std::cout << "Core: " << std::to_string(core)
                             << "  " << currentLine << " / " << totalLines;
                } else {
                    std::cout << "Finished " << totalLines << " / " << totalLines;
                }
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;
    }

    void displayHeader() {
        std::cout << CSOPESY_HEADER << std::endl;
        std::cout << "\033[32mHello! Welcome to CSOPESY commandline!\033[0m" << std::endl;
        std::cout << "\033[93mType 'exit' to quit, 'clear' to refresh the screen.\033[0m" << std::endl;
        std::cout << std::endl;
    }

    bool validateCommand(const std::string& command) {
        static const std::vector<std::string> validCommands = {
            "initialize", "screen -ls", "scheduler-start", "scheduler-stop", "report-util", "exit", "clear"
        };

        return (command.rfind("screen -s ", 0) == 0 ||
                command.rfind("screen -r ", 0) == 0 ||
                std::find(validCommands.begin(), validCommands.end(), command) != validCommands.end());
    }

    void executeCommand(const std::string& command) {

        if (handleScreenCommand(command)) {
            return;
        }

        if (command == "screen -ls") {
            displayProcessStatus();
        }
        else if (command == "scheduler-start") {
            scheduler.start();
            if (!generating) {
                generating = true;
                processGeneratorThread = std::thread(&OpesyConsole::processGenerationLoop, this);
            }
            std::cout << "Scheduler started. Dummy process generation initiated." << std::endl;
        }
        else if (command == "scheduler-stop") {
            if (generating) {
                generating = false;
                if (processGeneratorThread.joinable()) {
                    processGeneratorThread.join();
                }
            }
            scheduler.stop();
            std::cout << "Scheduler and dummy process generation stopped." << std::endl;
        }
        else {
            std::cout << command << " command recognized. Doing something." << std::endl;
        }
    }

    void displayProcessStatus() const {
        int totalCores = Scheduler::NUM_CORES;
        int usedCores = 0;
        for (int i = 0; i < totalCores; ++i) {
            if (scheduler.isCoreBusy(i)) ++usedCores;
        }
        int availableCores = totalCores - usedCores;
        int cpuUtil = (totalCores == 0) ? 0 : (usedCores * 100 / totalCores);
        std::cout << "CPU utilization: " << cpuUtil << "%\n";
        std::cout << "Cores used: " << usedCores << "\n";
        std::cout << "Cores available: " << availableCores << "\n";
        std::cout << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        const_cast<OpesyConsole*>(this)->displayProcessesByStatus(true);  // Running processes
        const_cast<OpesyConsole*>(this)->displayProcessesByStatus(false); // Finished processes
        std::cout << "----------------------------------------" << std::endl;
    }

    void sessionLoop(const std::string& sessionName, int pid) {
        auto process = processManager.getProcess(pid);
        if (!process) return;

        std::string input;
        while (true) {
            clearScreen();
            displayProcessInfo(sessionName, pid, false);
            std::cout << "root:/> ";
            std::getline(std::cin, input);

            if (input == "exit") {
                clearScreen();
                displayHeader();
                return;
            }

            if (input == "process-smi") {
                clearScreen();
                displayProcessInfo(sessionName, pid, true);
                continue;
            }

            // Execute process instruction
            std::string result = processManager.executeProcessInstruction(pid);

            if (process->isComplete()) {
                clearScreen();
                displayProcessInfo(sessionName, pid, false);
                std::cout << "Process completed. Press Enter to continue..." << std::endl;

                // Flush stdin just in case
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                // Wait for Enter
                std::getline(std::cin, input);  // This is better than cin.get() in this case

                clearScreen();
                displayHeader();
                return;
            }
        }
    }

    void processCommand(const std::string& command) {
        if (command == "exit") {
            std::cout << "Exiting CSOPESY CLI..." << std::endl;
            if (generating) {
                generating = false;
                if (processGeneratorThread.joinable()) {
                    processGeneratorThread.join();
                }
            } // if it is still generating processes it should stop on exit
            exit(0);
        }

        if (command == "initialize") {
            initialized = readConfigFromFile("config.txt", config);
            if (initialized) {
                std::cout << "\n System successfully initialized from config.txt:\n";
                std::cout << "num-cpu: " << config.numCPU << '\n';
                std::cout << "scheduler: " << config.scheduler << '\n';
                std::cout << "quantum-cycles: " << config.quantumCycles << '\n';
                std::cout << "batch-process-freq: " << config.batchProcessFreq << '\n';
                std::cout << "min-ins: " << config.minInstructions << '\n';
                std::cout << "max-ins: " << config.maxInstructions << '\n';
                std::cout << "delay-per-exec: " << config.delaysPerExec << "\n\n";
            } else {
                std::cout << "Failed to initialize system from config.txt\n";
            }
            return;
        }

        if (!initialized) {
            std::cout << "System not initialized. Please run 'initialize' before using any other command.\n";
            return;
        }

        if (command == "clear") {
            clearScreen();
            displayHeader();
        } else if (validateCommand(command)) {
            executeCommand(command);
        } else {
            std::cout << command << " command unrecognized. Enter a valid command." << std::endl;
        }

        std::cout << std::endl;
    }

public:
    void run() {
        std::string command;
        displayHeader();

        while (true) {
            std::cout << "Enter a command: ";
            std::getline(std::cin, command);
            processCommand(command);
        }
    }
};

#endif
