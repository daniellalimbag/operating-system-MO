#ifndef CONSOLE_H
#define CONSOLE_H
#include <iostream>
#include <filesystem>
#include <string>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <ctime>
#include <thread>
#include <atomic>
#include <random>
#include <mutex>
#include <chrono>
#ifdef _WIN32
#include <conio.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <algorithm>
#include <windows.h>
#endif
#include "Header.h"
#include "Process.h"
#include "ProcessManager.h"
#include "Scheduler.h"
#include "Config.h"
#include "ProcessInstruction.h"
#include "MarqueeConsole.h"
#include "FirstFitMemoryAllocator.h"
#include <set>
#include <fstream>

extern std::atomic<uint64_t> cpuTickCount;
class OpesyConsole {
private:
    std::unique_ptr<FirstFitMemoryAllocator> memoryAllocator;
    int quantumCycle = 0;
private:
    ProcessManager processManager;
    Scheduler scheduler{processManager};
    void setupMemorySnapshotCallback() {
        scheduler.setMemorySnapshotCallback([this](uint64_t tick) { 
            static int snapshotCounter = 1;
            this->outputMemorySnapshot(snapshotCounter++); 
        });
    }
    SystemConfig config;
    bool initialized = false;
    std::thread processGeneratorThread;
    std::atomic<bool> generating = false;
    int processCounter = 1;
    std::mt19937 rng{std::random_device{}()};
    std::unique_ptr<MarqueeConsole> marqueeConsole;

    bool validateCommand(const std::string& command) {
        static const std::vector<std::string> validCommands = {
            "initialize", "screen -ls", "scheduler-start", "scheduler-stop", 
            "report-util", "exit", "clear", "marquee"
        };
        return (command.rfind("screen -s ", 0) == 0 ||
                command.rfind("screen -r ", 0) == 0 ||
                std::find(validCommands.begin(), validCommands.end(), command) != validCommands.end());
    }

void processGenerationLoop() {
    static uint64_t lastProcessGenerationTick = 0;
    
    while (generating) {
        uint64_t currentTick = cpuTickCount.load();
        
        // Generate process based on batch frequency
        if (currentTick - lastProcessGenerationTick >= config.batchProcessFreq) {
            lastProcessGenerationTick = currentTick;
            std::ostringstream oss;
            oss << "p" << std::setw(2) << std::setfill('0') << processCounter++;
            std::string name = oss.str();
            int pid = processManager.createProcess(name);
            Process* proc = processManager.getProcess(pid);
            
            if (proc) {
                generateRandomInstructions(proc);
                if (!memoryAllocator->allocate(pid)) {
                    scheduler.addProcess(pid);
                    continue;
                }
                scheduler.addProcess(pid);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

    void generateRandomInstructions(Process* proc) {
        int instructionCount = std::uniform_int_distribution<int>(
            config.minInstructions, config.maxInstructions)(rng);

        std::vector<std::string> declaredVars;
        std::vector<std::unique_ptr<IProcessInstruction>> instructions;

        // Prepare DECLARE instructions
        int numVars = std::max(1, std::uniform_int_distribution<int>(1, 3)(rng));
        for (int v = 0; v < numVars; ++v) {
            std::string var = "v" + std::to_string(v + 1);
            uint16_t value = std::uniform_int_distribution<uint16_t>(0, 100)(rng);
            declaredVars.push_back(var);
            instructions.push_back(std::make_unique<DeclareInstruction>(var, value));
        }

        // Other instructions
        int i = 0;
        while (i < instructionCount) {
            int choice = std::uniform_int_distribution<int>(0, 5)(rng);
            switch (choice) {
                case 0: // ADD
                    if (!declaredVars.empty()) {
                        // 3 variables to use
                        std::string var1 = declaredVars.size() > 0 ? declaredVars[0] : "v1";
                        std::string var2 = declaredVars.size() > 1 ? declaredVars[1] : "v2";
                        std::string var3 = declaredVars.size() > 2 ? declaredVars[2] : "v3";
                        instructions.push_back(std::make_unique<AddInstruction>(var1, var2, var3));
                        ++i;
                    }
                    break;

                case 1: // SUBTRACT
                    if (!declaredVars.empty()) {
                        std::string var1 = declaredVars.size() > 0 ? declaredVars[0] : "v1";
                        std::string var2 = declaredVars.size() > 1 ? declaredVars[1] : "v2";
                        std::string var3 = declaredVars.size() > 2 ? declaredVars[2] : "v3";
                        instructions.push_back(std::make_unique<SubtractInstruction>(var1, var2, var3));
                        ++i;
                    }
                    break;

                case 2: // SLEEP
                    {
                        int ticks = std::uniform_int_distribution<int>(1, 5)(rng);
                        instructions.push_back(std::make_unique<SleepInstruction>(ticks));
                        ++i;
                    }
                    break;

                case 3: // PRINT
                    {
                        std::string message = "Hello world from " + proc->getProcessName() + "!";
                        instructions.push_back(std::make_unique<PrintInstruction>(message));
                        ++i;
                    }
                    break;

                case 4: // FOR
                    if (i + 4 < instructionCount && !declaredVars.empty()) {
                        int bodyCount = std::min(instructionCount - i - 2, std::uniform_int_distribution<int>(2, 3)(rng));
                        std::vector<std::unique_ptr<IProcessInstruction>> bodyInstructions;
                        for (int b = 0; b < bodyCount; ++b) {
                            int bodyChoice = std::uniform_int_distribution<int>(0, 2)(rng);
                            switch (bodyChoice) {
                                case 0:
                                    {
                                        std::string var1 = declaredVars.size() > 0 ? declaredVars[0] : "v1";
                                        std::string var2 = declaredVars.size() > 1 ? declaredVars[1] : "v2";
                                        std::string var3 = declaredVars.size() > 2 ? declaredVars[2] : "v3";
                                        bodyInstructions.push_back(std::make_unique<AddInstruction>(var1, var2, var3));
                                    }
                                    break;
                                case 1:
                                    {
                                        std::string var1 = declaredVars.size() > 0 ? declaredVars[0] : "v1";
                                        std::string var2 = declaredVars.size() > 1 ? declaredVars[1] : "v2";
                                        std::string var3 = declaredVars.size() > 2 ? declaredVars[2] : "v3";
                                        bodyInstructions.push_back(std::make_unique<SubtractInstruction>(var1, var2, var3));
                                    }
                                    break;
                                default:
                                    {
                                        std::string message = "Hello world from " + proc->getProcessName() + "!";
                                        bodyInstructions.push_back(std::make_unique<PrintInstruction>(message));
                                    }
                                    break;
                            }
                        }
                        int repeats = std::uniform_int_distribution<int>(2, 4)(rng);
                        instructions.push_back(std::make_unique<ForInstruction>(std::move(bodyInstructions), repeats));
                        i += 2;
                    } else {
                        std::string message = "Hello world from " + proc->getProcessName() + "!";
                        instructions.push_back(std::make_unique<PrintInstruction>(message));
                        ++i;
                    }
                    break;

                default:
                    {
                        std::string message = "Hello world from " + proc->getProcessName() + "!";
                        instructions.push_back(std::make_unique<PrintInstruction>(message));
                        ++i;
                    }
                    break;
            }
        }

        // Shuffle all instructions
        std::shuffle(instructions.begin(), instructions.end(), rng);

        // Add all instructions to the process
        for (auto& instr : instructions) {
            proc->addInstruction(std::move(instr));
        }
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
        std::string message = "Hello world from " + proc->getProcessName() + "!";
        proc->addInstruction(std::make_unique<PrintInstruction>(message));
    }

    void generatePrintInstructionWithMessage(Process* proc, const std::string& processName, int lineNum) {
        std::string message = "Hello world from " + processName + "!";
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
                    {
                        std::string message = "Hello world from " + proc->getProcessName() + "!";
                        bodyInstructions.push_back(std::make_unique<PrintInstruction>(message));
                    }
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

        std::cout << "ID: " << pid << std::endl;
        std::cout << "Logs:" << std::endl;

        std::string logs = process->getLogs();
        std::cout << (logs.empty() ? "No logs available yet." : logs) << std::endl;

        if (process->isComplete()) {
            std::cout << "Finished!" << std::endl;
        } else {
            std::cout << "Current instruction line: " << process->getCurrentInstructionNumber()
                     << " / " << process->countEffectiveInstructions() << std::endl;
        }
        std::cout << std::endl;
    }

    void outputProcessesByStatus(std::ostream& out, bool showRunning) const {
        auto processes = processManager.getAllProcesses();
        out << (showRunning ? "Running" : "Finished") << " processes:" << std::endl;

        // Gather all PIDs currently assigned to a core
        std::set<int> runningPIDs;
        int totalCores = config.numCPU;
        for (int i = 0; i < totalCores; ++i) {
            int pid = scheduler.getCoreProcess(i);
            if (pid != -1) runningPIDs.insert(pid);
        }

        for (const auto& process : processes) {
            if (showRunning) {
                if (runningPIDs.count(process->getPID()) == 0) continue;
            } else {
                if (!process->isComplete()) continue;
            }
            auto timestamp = process->getTimestamp();
            out << process->getProcessName() << "\t(" << timestamp << ")\t";
            size_t currentLine = process->getCurrentInstructionNumber();
            size_t totalLines = process->countEffectiveInstructions();
            if (showRunning) {
                out << "Core: " << std::to_string(process->getCore())
                    << "  " << currentLine << " / " << totalLines;
            } else {
                out << "Finished " << totalLines << " / " << totalLines;
            }
            out << std::endl;
        }
        out << std::endl;
    }

    void displayProcessesByStatus(bool showRunning) const {
        outputProcessesByStatus(std::cout, showRunning);
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
            if (!generating) {
                generating = true;
                processGeneratorThread = std::thread(&OpesyConsole::processGenerationLoop, this);
                std::cout << "Dummy process generation started." << std::endl;
            } else {
                std::cout << "Dummy process generation is already running." << std::endl;
            }
        }
        else if (command == "scheduler-stop") {
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
        else if (command == "marquee") {
            runMarqueeConsole();
        }
        else {
            std::cout << command << " command recognized. Doing something." << std::endl;
        }
    }

    void runMarqueeConsole() {
        if (!marqueeConsole) {
            marqueeConsole = std::make_unique<MarqueeConsole>();
            
            marqueeConsole->setExitCallback([this]() {
                clearScreen();
                displayHeader();
            });
        }
        
        marqueeConsole->run();
    }

    void generateUtilizationReport() {
        std::ofstream logFile("csopesy-log.txt");
        if (!logFile.is_open()) {
            std::cout << "Error: Could not create csopesy-log.txt" << std::endl;
            return;
        }

        int totalCores = config.numCPU;
        int usedCores = 0;
        for (int i = 0; i < totalCores; ++i) {
            if (scheduler.isCoreBusy(i)) ++usedCores;
        }
        int availableCores = totalCores - usedCores;
        int cpuUtil = (totalCores == 0) ? 0 : (usedCores * 100 / totalCores);

        logFile << "CPU utilization: " << cpuUtil << "%\n";
        logFile << "Cores used: " << usedCores << "\n";
        logFile << "Cores available: " << availableCores << "\n";
        logFile << std::endl;
        logFile << "----------------------------------------" << std::endl;
        outputProcessesByStatus(logFile, true);
        outputProcessesByStatus(logFile, false);
        logFile << "----------------------------------------" << std::endl;

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
        clearScreen();
        while (true) {
            displayProcessInfo(sessionName, pid, false);
            std::cout << "root:/> ";
            std::getline(std::cin, input);

            if (input == "exit") {
                clearScreen();
                displayHeader();
                return;
            }

            if (input == "process-smi") {
                displayProcessInfo(sessionName, pid, true);
                continue;
            }
            if (!input.empty()) {
                std::cout << "Invalid command." << std::endl;
            }

            if (process->isComplete()) {
                memoryAllocator->release(pid);
                clearScreen();
                displayProcessInfo(sessionName, pid, false);
                std::cout << "Process completed." << std::endl;
                std::cin.clear();
                continue;
            }
        }
    }

void outputMemorySnapshot(int quantumCycle) {
    std::string dirName = "memory_stamps";
    
    std::filesystem::create_directory(dirName);
    
    std::ostringstream filename;
    filename << dirName << "/memory_stamp_" << std::setw(2) << std::setfill('0') << quantumCycle << ".txt";
    
    std::ofstream out(filename.str());
    if (!out.is_open()) {
        std::cerr << "Failed to create memory snapshot file: " << filename.str() << std::endl;
        return;
    }
    
    // Timestamp
    std::time_t now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%d/%m/%Y %I:%M:%S%p", std::localtime(&now));
    out << "Timestamp: (" << buf << ")\n";
    out << "Number of processes in memory: " << memoryAllocator->getNumProcessesInMemory() << "\n";
    out << "Total external fragmentation in KB: " << (memoryAllocator->getExternalFragmentation() / 1024) << "\n\n";
    memoryAllocator->printMemory(out);
    out.close();
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
        static bool snapshotCallbackSet = false;
        if (!snapshotCallbackSet) { setupMemorySnapshotCallback(); snapshotCallbackSet = true; }
        if (handleScreenCommand(command)) return;
        if (command == "exit") {
            std::cout << "Exiting CSOPESY CLI..." << std::endl;
            if (generating) {
                generating = false;
                if (processGeneratorThread.joinable()) {
                    processGeneratorThread.join();
                }
            }
            scheduler.stop();
            exit(0);
        }

        if (command == "initialize") {
            initialized = readConfigFromFile("config.txt", config);
            if (initialized) {
                quantumCycle = 0;
                memoryAllocator = std::make_unique<FirstFitMemoryAllocator>(config.maxOverallMem, config.memPerProc);
globalMemoryAllocator = memoryAllocator.get();
                memoryAllocator = std::make_unique<FirstFitMemoryAllocator>(config.maxOverallMem, config.memPerProc);
globalMemoryAllocator = memoryAllocator.get();
                scheduler.updateConfig(config);
                scheduler.start();
                
                std::cout << "\nSuccessfully initialized from config.txt:\n";
                std::cout << "num-cpu: " << config.numCPU << '\n';
                std::cout << "scheduler: " << config.scheduler << '\n';
                std::cout << "quantum-cycles: " << config.quantumCycles << '\n';
                std::cout << "batch-process-freq: " << config.batchProcessFreq << '\n';
                std::cout << "min-ins: " << config.minInstructions << '\n';
                std::cout << "max-ins: " << config.maxInstructions << '\n';
                std::cout << "delay-per-exec: " << config.delaysPerExec << '\n';
                std::cout << "max-overall-mem: " << config.maxOverallMem << '\n';
                std::cout << "mem-per-frame: " << config.memPerFrame << '\n';
                std::cout << "mem-per-proc: " << config.memPerProc << "\n\n";
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