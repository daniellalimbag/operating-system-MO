#ifndef CONSOLE_H
#define CONSOLE_H

#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <ctime>
#include <conio.h>
#include "Header.h"
#include "Process.h"
#include "ProcessManager.h"
#include "Scheduler.h"
#include "Config.h"
#include <fstream>
#include <sstream>

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
            std::string name = "p" + std::string(processCounter < 10 ? "0" : "") + std::to_string(processCounter++);
            int pid = processManager.createProcess(name);
            Process* proc = processManager.getProcess(pid);
            if (proc) {
                int instructionCount = rand() % (config.maxInstructions - config.minInstructions + 1) + config.minInstructions;
                for (int i = 0; i < instructionCount; ++i) {
                    proc->addInstruction("PRINT(\"Hello from " + name + " - line " + std::to_string(i + 1) + "\")");
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

    bool readConfigFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open config.txt\n";
            return false;
        }

        std::string key;
        while (file >> key) {
            if (key == "num-cpu") file >> config.numCPU;
            else if (key == "scheduler") file >> config.scheduler;
            else if (key == "quantum-cycles") file >> config.quantumCycles;
            else if (key == "batch-process-freq") file >> config.batchProcessFreq;
            else if (key == "min-ins") file >> config.minInstructions;
            else if (key == "max-ins") file >> config.maxInstructions;
            else if (key == "delay-per-exec") file >> config.delaysPerExec;
            else {
                std::cerr << "Warning: Unknown config parameter: " << key << "\n";
                std::string dummy;
                file >> dummy;
            }
        }

        file.close();
        return true;
    }


    void clearAndDisplayHeader() {
        system("cls");
        displayHeader();
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
                
                if (showRunning) {
                    std::cout << "Core: " << std::to_string(core)
                             << "  " << process->getCPUUtilization() << " / 100";
                } else {
                    std::cout << "Finished 100 / 100";
                }
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;
    }

public:
    OpesyConsole() {
        // Hardcoded: Create 10 processes, each with 100 print commands (To be removed: This is for the week 6 homework)
        /*for (int i = 1; i <= 10; ++i) {
            std::string pname = std::string("process") + (i < 10 ? "0" : "") + std::to_string(i);
            int pid = processManager.createProcess(pname);
            Process* proc = processManager.getProcess(pid);
            
            if (proc) {
                for (int j = 1; j <= 100; ++j) {
                    proc->addInstruction("PRINT(\"Hello world from " + pname + "!\")");
                }
                proc->addInstruction("EXIT");
            }
            scheduler.addProcess(pid);
        }
        scheduler.start();*/
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
            system("cls");
            displayProcessInfo(sessionName, pid, false);
            std::cout << "root:/> ";
            std::getline(std::cin, input);

            if (input == "exit") {
                clearAndDisplayHeader();
                return;
            }

            if (input == "process-smi") {
                system("cls");
                displayProcessInfo(sessionName, pid, true);
                continue;
            }

            // Execute process instruction
            std::string result = processManager.executeProcessInstruction(pid);

            if (process->isComplete()) {
                system("cls");
                displayProcessInfo(sessionName, pid, false);
                std::cout << "Process completed. Press Enter to continue..." << std::endl;

                // Flush stdin just in case
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                // Wait for Enter
                std::getline(std::cin, input);  // This is better than cin.get() in this case

                clearAndDisplayHeader();
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
            initialized = readConfigFromFile("config.txt");
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
            clearAndDisplayHeader();
        } else if (validateCommand(command)) {
            executeCommand(command);
        } else {
            std::cout << command << " command unrecognized. Enter a valid command." << std::endl;
        }

        std::cout << std::endl;
    }


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