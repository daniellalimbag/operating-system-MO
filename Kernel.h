#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <map>
#include <cstdint>
#include <queue>

#include "Process.h"
#include "SystemConfig.h"
#include "ProcessInstruction.h"

/**
 * @struct CPUCore
 * @brief Represents a single CPU core in the emulator.
 */
struct CPUCore {
    uint32_t id; // Unique identifier for the core (e.g., 0, 1, 2...)
    Process* currentProcess; // Pointer to the process currently running on this core
    bool isBusy;             // True if the core is currently executing a process
    uint32_t currentQuantumTicks; // New: Tracks ticks consumed in current quantum for currentProcess
};

/**
 * @class Kernel
 * @brief Core OS kernel responsible for OS management, and exposes public API for system interaction
 */
// Kernel.h
class Kernel {
public:
    Kernel();

    // Core OS Lifecycle Methods
    void initialize(const SystemConfig& config);
    void shutdown();
    void run();

    // Process Management
    // int createProcess(std::vector<std::unique_ptr<IProcessInstruction>> instructions);

    // Process Generation Control
    void startProcessGeneration();      // scheduler-start
    void stopProcessGeneration();       // scheduler-stop

    // Screen Commands
    void listStatus() const;                                            // screen -ls
    Process* reattachToProcess(const std::string& processName) const;   // screen -r
    Process* startProcess(const std::string& processName);              // screen -s
    void printSmi(Process* process) const;                              // process-smi

    // System Clock & Time
    unsigned long long getCurrentCpuTick() const;

    // I/O APIs
    void print(const std::string& message) const;
    std::string readLine(const std::string& prompt) const;
    void clearScreen() const;

private:
    std::vector<std::unique_ptr<Process>> m_processes;  // Stores all processes managed by the kernel
    int m_nextPid;                                      // Counter for assigning unique PIDs
    unsigned long long m_cpuTicks;                      // Represents the system's conceptual time progression
    std::atomic<bool> m_isInitialized;                  // Initialized flag for run() to wait before initialization

    mutable std::mutex m_kernelMutex;           // Mutex to protect access to Kernel's shared data members
    std::atomic<bool> m_runningGeneration;      // Atomic flag to signal the kernel thread to stop generating dummy processes
    std::atomic<bool> m_shutdownRequested;      // Atomic flag to signal that a shutdown has been requested
    std::condition_variable m_cv;               // Condition variable for signaling idle/active state

    // Configuration Parameters
    uint32_t m_numCpus;
    SchedulerType m_schedulerType;
    uint32_t m_quantumCycles;
    uint32_t m_batchProcessFreq;
    uint32_t m_minInstructions;
    uint32_t m_maxInstructions;
    uint32_t m_delaysPerExec;
    uint32_t m_maxOverallMem;
    uint32_t m_memPerFrame;
    uint32_t m_minMemPerProc;
    uint32_t m_maxMemPerProc;

    std::vector<CPUCore> m_cpuCores;            // Virtual representation of CPU cores
    std::queue<Process*> m_readyQueue;          // Virtual representation of the ready queue for scheduling
    std::vector<Process*> m_waitingProcesses;   // Pointers to handle waiting processes before putting in the ready queue

    // Internal Kernel Operations
    void scheduleProcesses();                                               // Selects and runs a process
    void updateWaitingProcesses();                                          // Update status of waiting processes (e.g., sleeping)
    bool isBusy();                                                          // Checks if any core is busy or if there are still processes in the ready queue
    Process* generateDummyProcess(const std::string& newPname, int newPid); // Dummy Process Generation helper
};
