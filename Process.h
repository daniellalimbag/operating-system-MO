// Process.h
#pragma once

#include <string>
#include <vector>
#include <chrono>

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
 * @struct ProcessInstruction
 * @brief Placeholder for a single instruction that a process can execute.
 * Detailed instruction types and arguments will be added later.
 */
struct ProcessInstruction {
    std::string original_line; // A simple string to represent the instruction for now. This will be replaced by enums, specific arguments, etc., later.

    ProcessInstruction(const std::string& line = "GENERIC_INSTRUCTION") : original_line(line) {}
};

/**
 * @class Process
 * @brief Represents a single process in the Simple OS.
 * @details This initial version focuses on identity, state, control flow, and lifecycle.
 * Other functionalities (variables, logging, complex instructions) are deferred.
 */
class Process {
public: // Public interface for Kernel interaction
    Process(int id, const std::vector<ProcessInstruction>& cmds);

    // Core Lifecycle & Execution Methods
    void setState(ProcessState newState);
    bool isFinished() const;
    void executeNextInstruction(Kernel& kernel);

    // Public Getters for Read-Only Information
    int getPid() const { return m_pid; }
    ProcessState getState() const { return m_currentState; }
    std::chrono::system_clock::time_point getCreationTime() const { return m_creationTime; }
    std::chrono::system_clock::time_point getWakeUpTime() const { return m_wakeUpTime; }

    // Information for "screen" command
    int getCurrentInstructionLine() const { return m_programCounter; }
    int getTotalInstructionLines() const { return m_instructions.size(); }

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
    int m_pid;

    // Control Flow Variables
    ProcessState m_currentState;
    std::vector<ProcessInstruction> m_instructions;     // The list of commands for this process
    size_t m_programCounter;                            // Index of the next instruction to execute

    // Life-Cycle Timestamps
    std::chrono::system_clock::time_point m_creationTime;   // When the process was created
    std::chrono::system_clock::time_point m_wakeUpTime;     // Used specifically for SLEEP instruction (placeholder for now)
};
