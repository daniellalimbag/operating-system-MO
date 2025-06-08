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

class OpesyConsole {
private:
    ProcessManager processManager;
    Scheduler scheduler{processManager};

    // Helper methods to reduce repetition
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
            if (isRunning == showRunning) {
                auto timestamp = process->getTimestamp();
                std::cout << process->getProcessName() << "\t(" << timestamp << ")\t";
                
                if (showRunning) {
                    std::cout << "Core: " << (process->getCore() == -1 ? "N/A" : std::to_string(process->getCore()))
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
        // Create 10 processes, each with 100 print commands (To be removed: This is for the week 6 homework)
        for (int i = 1; i <= 10; ++i) {
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
        scheduler.start();
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
            std::cout << "Scheduler started." << std::endl;
        }
        else if (command == "scheduler-stop") {
            scheduler.stop();
            std::cout << "Scheduler stopped." << std::endl;
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
                break;
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
                std::cout << "Process completed. Press Enter to continue...";
                std::getline(std::cin, input);
                clearAndDisplayHeader();
                break;
            }
        }
    }

    void processCommand(const std::string& command) {
        if (command == "clear") {
            clearAndDisplayHeader();
        } else if (command == "exit") {
            std::cout << "Exiting CSOPESY CLI..." << std::endl;
            exit(0);
        } else {
            if (validateCommand(command)) {
                executeCommand(command);
            } else {
                std::cout << command << " command unrecognized. Enter a valid command." << std::endl;
            }
            std::cout << std::endl;
        }
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