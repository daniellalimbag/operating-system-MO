#ifndef CONSOLE_H
#define CONSOLE_H

#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <ctime>
#include <thread>
#include <atomic>
#include <random>
#ifdef _WIN32
#include <conio.h>
#endif
#include "Header.h"
#include "Process.h"
#include "ProcessManager.h"
#include "Scheduler.h"
#include "Config.h"
#include "ProcessInstruction.h"
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
    std::mt19937 rng{std::random_device{}()};

    bool validateCommand(const std::string& command) {
        static const std::vector<std::string> validCommands = {
            "initialize", "screen -ls", "scheduler-start", "scheduler-stop", "report-util", "exit", "clear"
        };
        return (command.rfind("screen -s ", 0) == 0 ||
                command.rfind("screen -r ", 0) == 0 ||
                std::find(validCommands.begin(), validCommands.end(), command) != validCommands.end());
    }

void processGenerationLoop() {
    uint64_t lastProcessGenTick = 0;
    
    while (generating) {
        uint64_t currentTick = cpuTickCount.load();
        
        // Generate process based on batch frequency
        if (config.batchProcessFreq > 0 && 
            (currentTick - lastProcessGenTick) >= static_cast<uint64_t>(config.batchProcessFreq * 10)) { //multiplied by 10 for now
            
            lastProcessGenTick = currentTick;
            
            std::ostringstream oss;
            oss << "p" << std::setw(2) << std::setfill('0') << processCounter++;
            std::string name = oss.str();
            int pid = processManager.createProcess(name);
            Process* proc = processManager.getProcess(pid);
            if (proc) {
                generateRandomInstructions(proc);
            }
            scheduler.addProcess(pid);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

    void generateRandomInstructions(Process* proc) {
        int instructionCount = std::uniform_int_distribution<int>(
            config.minInstructions, config.maxInstructions)(rng);
        
        std::vector<std::string> declaredVars;
        int numVars = std::max(1, std::uniform_int_distribution<int>(1, 3)(rng));
        
        // Declare variables first?? Idk if this is right
        for (int v = 0; v < numVars; ++v) {
            std::string var = "v" + std::to_string(v + 1);
            uint16_t initialValue = std::uniform_int_distribution<uint16_t>(0, 100)(rng);
            proc->addInstruction(std::make_unique<DeclareInstruction>(var, initialValue));
            proc->setVariableValue(var, initialValue);
            declaredVars.push_back(var);
        }
        
        // Generate random instructions
        int i = 0;
        while (i < instructionCount) {
            int choice = std::uniform_int_distribution<int>(0, 5)(rng);
            
            switch (choice) {
                case 0: // ADD
                    if (!declaredVars.empty()) {
                        generateAddInstruction(proc, declaredVars);
                        ++i;
                    } else {
                        generatePrintInstruction(proc);
                        ++i;
                    }
                    break;
                    
                case 1: // SUBTRACT
                    if (!declaredVars.empty()) {
                        generateSubtractInstruction(proc, declaredVars);
                        ++i;
                    } else {
                        generatePrintInstruction(proc);
                        ++i;
                    }
                    break;
                    
                case 2: // SLEEP
                    generateSleepInstruction(proc);
                    ++i;
                    break;
                    
                case 3: // PRINT
                    generatePrintInstructionWithMessage(proc, proc->getProcessName(), i + 1);
                    ++i;
                    break;
                    
                case 4: // FOR
                    if (i + 4 < instructionCount && !declaredVars.empty()) {
                        int bodyInstructions = generateForLoop(proc, declaredVars, instructionCount - i - 2);
                        i += bodyInstructions + 2;
                    } else {
                        generatePrintInstruction(proc);
                        ++i;
                    }
                    break;
                    
                default: 
                    generatePrintInstruction(proc);
                    ++i;
                    break;
            }
        }
        proc->addInstruction(std::make_unique<PrintInstruction>("Process completed."));
    }

    void generateAddInstruction(Process* proc, const std::vector<std::string>& vars) {
        // 3 variables to use
        std::string var1 = vars.size() > 0 ? vars[0] : "v1";
        std::string var2 = vars.size() > 1 ? vars[1] : "v2";
        std::string var3 = vars.size() > 2 ? vars[2] : "v3";
        proc->addInstruction(std::make_unique<AddInstruction>(var1, var2, var3));
    }

    void generateSubtractInstruction(Process* proc, const std::vector<std::string>& vars) {
        std::string var1 = vars.size() > 0 ? vars[0] : "v1";
        std::string var2 = vars.size() > 1 ? vars[1] : "v2";
        std::string var3 = vars.size() > 2 ? vars[2] : "v3";
        proc->addInstruction(std::make_unique<SubtractInstruction>(var1, var2, var3));
    }

    void generateSleepInstruction(Process* proc) {
        int ticks = std::uniform_int_distribution<int>(1, 5)(rng);
        proc->addInstruction(std::make_unique<SleepInstruction>(ticks));
    }

    void generatePrintInstruction(Process* proc) {
        proc->addInstruction(std::make_unique<PrintInstruction>("Hello from PRINT instruction"));
    }

    void generatePrintInstructionWithMessage(Process* proc, const std::string& processName, int lineNum) {
        std::string message = "Hello from " + processName + " - line " + std::to_string(lineNum);
        proc->addInstruction(std::make_unique<PrintInstruction>(message));
    }

    int generateForLoop(Process* proc, const std::vector<std::string>& vars, int maxBodyInstructions) {
        int bodyCount = std::min(maxBodyInstructions, std::uniform_int_distribution<int>(2, 3)(rng));
        std::vector<std::unique_ptr<IProcessInstruction>> bodyInstructions;
        for (int b = 0; b < bodyCount; ++b) {
            int bodyChoice = std::uniform_int_distribution<int>(0, 2)(rng);
            switch (bodyChoice) {
                case 0:
                    {
                        std::string var1 = vars.size() > 0 ? vars[0] : "v1";
                        std::string var2 = vars.size() > 1 ? vars[1] : "v2";
                        std::string var3 = vars.size() > 2 ? vars[2] : "v3";
                        bodyInstructions.push_back(std::make_unique<AddInstruction>(var1, var2, var3));
                    }
                    break;
                case 1:
                    {
                        std::string var1 = vars.size() > 0 ? vars[0] : "v1";
                        std::string var2 = vars.size() > 1 ? vars[1] : "v2";
                        std::string var3 = vars.size() > 2 ? vars[2] : "v3";
                        bodyInstructions.push_back(std::make_unique<SubtractInstruction>(var1, var2, var3));
                    }
                    break;
                default:
                    bodyInstructions.push_back(std::make_unique<PrintInstruction>("Hello from PRINT instruction"));
                    break;
            }
        }
        int repeats = std::uniform_int_distribution<int>(2, 4)(rng);
        proc->addInstruction(std::make_unique<ForInstruction>(std::move(bodyInstructions), repeats));
        return 1;
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

    void displayProcessesByStatus(bool showRunning) {
        auto processes = processManager.getAllProcesses();
        std::cout << (showRunning ? "Running" : "Finished") << " processes:" << std::endl;

        for (const auto& process : processes) {
            bool isRunning = !process->isComplete();
            int core = process->getCore();
            if (isRunning == showRunning) {
                if (showRunning && core == -1) continue;
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

    void executeCommand(const std::string& command) {
        if (command == "screen -ls") {
            displayProcessStatus();
        }
        else if (command == "scheduler-start") {
            // Only start process generation, scheduler should already be running
            if (!generating) {
                generating = true;
                processGeneratorThread = std::thread(&OpesyConsole::processGenerationLoop, this);
                std::cout << "Dummy process generation started." << std::endl;
            } else {
                std::cout << "Dummy process generation is already running." << std::endl;
            }
        }
        else if (command == "scheduler-stop") {
            // Only stop process generation, keep scheduler running
            if (generating) {
                generating = false;
                if (processGeneratorThread.joinable()) {
                    processGeneratorThread.join();
                }
                std::cout << "Dummy process generation stopped." << std::endl;
            } else {
                std::cout << "Dummy process generation is not running." << std::endl;
            }
        }
        else if (command == "report-util") {
            generateUtilizationReport();
        }
        else {
            std::cout << command << " command recognized. Doing something." << std::endl;
        }
    }

    void generateUtilizationReport() {
        std::ofstream logFile("csopesy-log.txt");
        if (!logFile.is_open()) {
            std::cout << "Error: Could not create csopesy-log.txt" << std::endl;
            return;
        }

        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm;
        #ifdef _WIN32
            localtime_s(&local_tm, &now_time_t);
        #else
            localtime_r(&now_time_t, &local_tm);
        #endif
        
        logFile << "CPU Utilization Report\n";
        logFile << "Timestamp: " << std::put_time(&local_tm, "%m/%d/%Y %I:%M:%S %p") << "\n";
        logFile << "Number of cores: " << config.numCPU << "\n";
        logFile << "Scheduler: " << config.scheduler << "\n\n";

        int totalCores = config.numCPU;
        int usedCores = 0;
        for (int i = 0; i < totalCores; ++i) {
            if (scheduler.isCoreBusy(i)) ++usedCores;
        }
        
        double utilization = (totalCores == 0) ? 0.0 : (static_cast<double>(usedCores) / totalCores * 100.0);
        
        logFile << "CPU Utilization: " << std::fixed << std::setprecision(2) << utilization << "%\n";
        logFile << "Cores used: " << usedCores << "\n";
        logFile << "Cores available: " << (totalCores - usedCores) << "\n\n";
        
        logFile << "Running processes:\n";
        auto processes = processManager.getAllProcesses();
        for (const auto& process : processes) {
            if (!process->isComplete() && process->getCore() != -1) {
                logFile << process->getProcessName() << " (Core " << process->getCore() 
                       << ") - " << process->getCurrentInstructionNumber() 
                       << "/" << process->getTotalInstructions() << " instructions\n";
            }
        }
        
        logFile << "\nFinished processes:\n";
        for (const auto& process : processes) {
            if (process->isComplete()) {
                logFile << process->getProcessName() << " - Completed " 
                       << process->getTotalInstructions() << " instructions\n";
            }
        }

        logFile.close();
        std::cout << "Report generated at csopesy-log.txt" << std::endl;
    }

    void displayProcessStatus() const {
        int totalCores = config.numCPU;
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
        const_cast<OpesyConsole*>(this)->displayProcessesByStatus(true);
        const_cast<OpesyConsole*>(this)->displayProcessesByStatus(false);
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

            if (!input.empty()) {
                process->addInstruction(std::make_unique<PrintInstruction>(input));
                std::cout << "Added instruction: " << input << std::endl;
            }

            if (process->isComplete()) {
                clearScreen();
                displayProcessInfo(sessionName, pid, false);
                std::cout << "Process completed. Press Enter to continue..." << std::endl;
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::getline(std::cin, input);
                clearScreen();
                displayHeader();
                return;
            }
        }
    }

    bool handleScreenCommand(const std::string& command) {
        if (command.rfind("screen -s ", 0) == 0) {
            std::string processName = command.substr(10);
            int pid = processManager.createProcess(processName);
            Process* proc = processManager.getProcess(pid);
            if (proc) {
                generateRandomInstructions(proc);
            }
            
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

    void processCommand(const std::string& command) {
        if (handleScreenCommand(command)) return;
        if (command == "exit") {
            std::cout << "Exiting CSOPESY CLI..." << std::endl;
            if (generating) {
                generating = false;
                if (processGeneratorThread.joinable()) {
                    processGeneratorThread.join();
                }
            }
            // Stop the scheduler when exiting
            scheduler.stop();
            exit(0);
        }

        if (command == "initialize") {
            initialized = readConfigFromFile("config.txt", config);
            if (initialized) {
                scheduler.updateConfig(config);
                // Start the scheduler automatically after initialization
                scheduler.start();
                
                std::cout << "\nSuccessfully initialized from config.txt:\n";
                std::cout << "num-cpu: " << config.numCPU << '\n';
                std::cout << "scheduler: " << config.scheduler << '\n';
                std::cout << "quantum-cycles: " << config.quantumCycles << '\n';
                std::cout << "batch-process-freq: " << config.batchProcessFreq << '\n';
                std::cout << "min-ins: " << config.minInstructions << '\n';
                std::cout << "max-ins: " << config.maxInstructions << '\n';
                std::cout << "delay-per-exec: " << config.delaysPerExec << "\n";
                std::cout << "Scheduler started automatically.\n\n";
            } else {
                std::cout << "Failed to initialize from config.txt\n";
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