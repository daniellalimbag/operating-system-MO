#ifdef _WIN32
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <queue>
#include <condition_variable>   
#include "SystemConfig.h"
#include "Process.h"

struct CPUCore {
    uint32_t id;
    Process* currentProcess;
    bool isBusy;
    uint32_t currentQuantumTicks;
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

    // Command APIs
    void startProcessGeneration();                                                  // scheduler-start
    void stopProcessGeneration();                                                   // scheduler-stop
    void listStatus() const;                                                        // screen -ls
    Process* reattachToProcess(const std::string& processName) const;               // screen -r
    Process* startProcess(const std::string& processName, uint32_t memRequired);    // screen -s
    Process* createUserDefinedProcess(const std::string& processName, const std::vector<std::string>& rawInstructions);    // screen -c
    void printSmi(Process* process) const;                                          // process-smi inside screen
    void printMemoryUtilizationReport() const;                                      // process-smi
    void printMemoryStatistics() const;                                             // vmstat
    void exportListStatusToFile(const std::string& filename) const;                 // report-util

    // I/O APIs
    void print(const std::string& message) const;
    std::string readLine(const std::string& prompt) const;
    void clearScreen() const;
    void printHorizontalLine() const;
    bool getIsInitialized() const { return m_isInitialized.load(); }

    // Public Memory Access APIs
    void handleMemoryAccess(Process& process, size_t virtualAddress);
    uint16_t readMemory(Process& process, size_t virtualAddress);
    void writeMemory(Process& process, size_t virtualAddress, uint16_t data);
    void dumpBackingStoreToFile(const std::string& filename) const;

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
    std::vector<uint16_t> m_physicalMemory;     // Virtual representation of physical memory
    std::vector<bool> m_frameStatus;            // Free frame list to keep track of which frame is available
    uint32_t m_totalFrames;

    // Internal Kernel Operations
    void updateWaitingQueue();
    void scheduleProcesses();
    bool executeAllCores();
    bool checkIfBusy();
    Process* generateDummyProcess(const std::string& newPname, uint32_t memRequired);
    void displayProcess(Process* process) const;
    ssize_t findFreeFrame() const;
    std::unique_ptr<IProcessInstruction> parseInstruction(const std::string& line);
};
