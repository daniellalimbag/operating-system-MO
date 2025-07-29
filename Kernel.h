#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
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

    // Process Generation Control
    void startProcessGeneration();      // scheduler-start
    void stopProcessGeneration();       // scheduler-stop

    // Screen Commands
    void listStatus() const;                                                        // screen -ls
    Process* reattachToProcess(const std::string& processName) const;               // screen -r
    Process* startProcess(const std::string& processName, uint32_t memRequired);    // screen -s
    void printSmi(Process* process) const;                                          // process-smi inside screen

    void printMemoryUtilizationReport() const;                                      // process-smi
    void printMemoryStatistics() const;                                             // vmstat

    // I/O APIs
    void print(const std::string& message) const;
    std::string readLine(const std::string& prompt) const;
    void clearScreen() const;

private:
    std::vector<std::unique_ptr<Process>> m_processes;  // Stores all processes managed by the kernel
    uint32_t m_nextPid;                                 // Counter for assigning unique PIDs
    uint64_t m_cpuTicks;                                // Represents the system's conceptual time progression
    std::atomic<bool> m_isInitialized;                  // Initialized flag for run() to wait before initialization
    uint64_t m_activeTicks;                             // Represents the amount of active CPU ticks for vmstat
    uint64_t m_numPagedIn;                              // Represents the amount of times pages were switched in
    uint64_t m_numPagedOut;                             // Represents the amount of times pages were switched out

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
    std::vector<Process*> m_waitingQueue;       // Pointers to handle waiting processes before putting in the ready queue
    std::vector<uint32_t> m_pageTable;          // Virtual representation of the page table. Index = Frame, Value = Process ID
    uint32_t m_totalFrames;

    // Internal Kernel Operations
    void scheduleProcesses();                                                               // Selects and runs a process
    void updateWaitingProcesses();                                                          // Update status of waiting processes (e.g., sleeping)
    bool isBusy();                                                                          // Checks if any core is busy or if there are still processes in the ready queue
    Process* generateDummyProcess(const std::string& newPname, uint32_t memRequired);       // Dummy Process Generation helper
    void displayProcess(Process* process) const;                                            // Prints the details of the process for the screen commands and print-smi
};
