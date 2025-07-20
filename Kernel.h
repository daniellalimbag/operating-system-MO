#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <map>
#include <cstdint>

#include "Process.h"
#include "SystemConfig.h"
#include "ProcessInstruction.h"

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
    int createProcess(std::vector<std::unique_ptr<IProcessInstruction>> instructions);
    //int getProcessCurrentInstructionLine(int pid) const;
    //int getProcessTotalInstructionLines(int pid) const;

    // Process Generation Control
    void startProcessGeneration();
    void stopProcessGeneration();

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

    mutable std::mutex m_kernelMutex;   // Mutex to protect access to Kernel's shared data members
    std::atomic<bool> m_running;        // Atomic flag to signal the kernel thread to stop

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
    uint32_t m_memPerProc;

    // Internal Kernel Operations
    void advanceCpuTick();                      // Increments the internal CPU tick counter
    void scheduleProcesses();                   // Selects and runs a process
    Process* findProcessByPid(int pid) const;   // Private helper method to find a process by its PID
    void generateDummyProcess();                // Dummy Process Generation helper
};
