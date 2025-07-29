// Process.h
#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <map>

#include "ProcessInstruction.h"

class Kernel;

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
 * @struct LoopContext
 * @brief Holds the contextual information for an active FOR loop within a process.
 */
struct LoopContext {
    size_t currentInstructionIndexInBody; // Current index within the loop's own instruction vector (ForInstruction::getBody())
    int currentIteration;              // The current iteration (0 to repeats-1)
    int totalRepeats;                     // Total number of repetitions for this loop
    const ForInstruction* forInstructionPtr; // Pointer to the actual ForInstruction object to access its body

    /**
     * @brief Constructor for LoopContext.
     * @param repeats The total number of repetitions for this loop.
     * @param forInstr A pointer to the ForInstruction object that initiated this loop.
     */
    LoopContext(int repeats, const ForInstruction* forInstr)
        : currentInstructionIndexInBody(0UL), currentIteration(0), totalRepeats(repeats), forInstructionPtr(forInstr) {}
};

/**
 * @class Process
 * @brief Represents a single process in the Simple OS.
 * @details This initial version focuses on identity, state, control flow, and lifecycle.
 * Other functionalities (variables, logging, complex instructions) are deferred.
 */
class Process {
public: // Public interface for Kernel interaction
    Process(uint32_t id, std::string processName, uint32_t memoryRequired , std::vector<std::unique_ptr<IProcessInstruction>>&& cmds);

    // Core Lifecycle & Execution Methods
    void setState(ProcessState newState);
    bool isFinished() const;
    void executeNextInstruction(uint32_t coreId);

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

    void decrementSleepTicks() { if (m_sleepTicksRemaining > 0) m_sleepTicksRemaining--; }
    void setSleepTicks(uint8_t ticks);
    void declareVariable(const std::string& varName, uint16_t value);
    uint16_t getVariableValue(const std::string& operand);
    void setVariableValue(const std::string& varName, uint16_t value);
    uint16_t clampUint16(int value);
    void addToLog(const std::string& message);
    const std::vector<std::string>& getLogBuffer() const { return m_logBuffer; }
    void pushLoopContext(const ForInstruction* forInstr);
    const std::vector<LoopContext>& getLoopStack() const { return m_loopStack; }

    // =====================================================================
    // Specialized helper objects (e.g., SymbolTable, ProcessLogger)
    // and their associated data/methods will be placed here in future versions
    // after initial core process functionality is established.
    // For example:
    // std::map<std::string, uint16_t> variables; // For process's local variables
    // std::vector<LogEntry> log_buffer;          // For process's screen output
    // std::vector<LoopContext> loop_stack;       // For FOR loop management
    // =====================================================================
private:
    uint32_t m_pid;
    std::string m_processName;
    uint32_t m_memoryRequired;

    // Control Flow Variables
    ProcessState m_currentState;
    std::vector<std::unique_ptr<IProcessInstruction>> m_instructions;   // The list of commands for this process
    size_t m_programCounter;                                            // Index of the next instruction to execute

    std::chrono::system_clock::time_point m_creationTime;   // When the process was created

    uint8_t m_sleepTicksRemaining;                          // Used specifically for SLEEP instruction

    // Process-specific data
    std::map<std::string, uint16_t> m_variables; // For process's local variables
    std::vector<std::string> m_logBuffer;       // For process's screen output (PRINT statements)
    std::vector<LoopContext> m_loopStack;       // For FOR loop management

    // Current execution context
    int m_currentExecutionCoreId;
};
