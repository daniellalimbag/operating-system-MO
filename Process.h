// Process.h
#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <algorithm>
#include <cctype>

#include "ProcessInstruction.h"
#include "SystemConfig.h"

class Kernel; // Forward declaration

/**
 * @enum ProcessState
 * @brief Defines the possible states a process can be in.
 */
enum class ProcessState {
    NEW,        ///< Process has been created but not yet admitted for execution.
    READY,      ///< Process is ready to run and waiting for the CPU.
    RUNNING,    ///< Process is currently executing on the CPU.
    WAITING,    ///< Process is waiting for some event (e.g., sleep duration).
    TERMINATED  ///< Process has finished execution.
};

/**
 * @class Process
 * @brief Represents a single process in the Simple OS.
 * @details This version implements virtual memory for process variables.
 */
class Process {
public: // Public interface for Kernel interaction
    Process(uint32_t id, std::string processName, uint32_t memoryRequired , std::vector<std::unique_ptr<IProcessInstruction>>&& cmds);

    // Core Lifecycle & Execution Methods
    void setState(ProcessState newState);
    bool isFinished() const;
    void executeNextInstruction(uint32_t coreId, Kernel& kernel);

    // Public Getters for Read-Only Information
    uint32_t getPid() const { return m_pid; }
    std::string getPname() const { return m_processName; }
    ProcessState getState() const { return m_currentState; }
    std::chrono::system_clock::time_point getCreationTime() const { return m_creationTime; }
    uint8_t getSleepTicksRemaining() const { return m_sleepTicksRemaining; }
    size_t getCurrentInstructionLine() const { return m_programCounter; }
    size_t getTotalInstructionLines() const { return m_instructions.size(); }
    uint32_t getCurrentExecutionCoreId() const { return m_currentExecutionCoreId; }
    uint32_t getMemoryRequired() const { return m_memoryRequired; }

    std::map<size_t, size_t>& getPageTable() { return m_pageTable; }

    // New and updated methods for variable handling
    void allocateVariable(const std::string& varName);
    bool hasVariable(const std::string& varName) const;
    size_t getVirtualAddressForVariable(const std::string& varName) const;

    void decrementSleepTicks() { if (m_sleepTicksRemaining > 0) m_sleepTicksRemaining--; }
    void setSleepTicks(uint8_t ticks);
    uint16_t clampUint16(int value);
    void addToLog(const std::string& message);
    const std::vector<std::string>& getLogBuffer() const { return m_logBuffer; }
    bool isNumeric(const std::string& str) const;

private:
    uint32_t m_pid;
    std::string m_processName;
    uint32_t m_memoryRequired;
    std::map<size_t, size_t> m_pageTable;
    std::map<std::string, size_t> m_variableAddresses; // Maps var name to its virtual address
    size_t m_nextVirtualAddressOffset;                 // The next available virtual address

    // Control Flow Variables
    ProcessState m_currentState;
    std::vector<std::unique_ptr<IProcessInstruction>> m_instructions;   // The list of commands for this process
    size_t m_programCounter;                                            // Index of the next instruction to execute

    std::chrono::system_clock::time_point m_creationTime;   // When the process was created

    uint8_t m_sleepTicksRemaining;                          // Used specifically for SLEEP instruction

    // Process-specific data
    std::vector<std::string> m_logBuffer;       // For process's screen output (PRINT statements)

    // Current execution context
    int m_currentExecutionCoreId;
};
