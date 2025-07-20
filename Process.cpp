#include "Process.h"
#include "Kernel.h"

#include <iostream>
#include <string>

// ======================================================================================
// Anonymous Namespace for Internal Helper Functions (Instruction Execution Logic)
// Functions declared here are only visible within this Process.cpp file.
// ======================================================================================
namespace {

    void executeSingleInstruction(const ProcessInstruction& instruction, Process& process, Kernel& kernel) {
        // For now, ProcessInstruction is very simple, so we just print its original_line.
        std::cout << "[Process " << process.getPid() << " - Internal Handler] Executing: " << instruction.original_line << std::endl;

        // Example of what would happen here when instruction types are fleshed out:
        /*
        switch (instruction.type) {
            case ProcessInstruction::Type::PRINT: {
                // Logic for PRINT (e.g., extracting message from instruction.raw_args)
                // Example: kernel.print(process.getPid(), instruction.getPrintMessage());
                break;
            }
            case ProcessInstruction::Type::DECLARE: {
                // Logic for DECLARE (e.g., process.setVariable(name, value))
                // Note: If 'setVariable' is a public method for the Process to manipulate its own internal state,
                // or if Process itself has a private member function to handle it.
                break;
            }
            // ... and so on for ADD, SUBTRACT, SLEEP, FOR, READ, WRITE
            default:
                std::cerr << "Error: Unknown instruction type encountered for Process " << process.getPid() << std::endl;
                // Optionally, set process state to ERROR or TERMINATED via process.setState()
                break;
        }
        */
        // For a SLEEP instruction, this function would update process.wake_up_time
        // For example: process.wake_up_time = kernel.getCurrentTick() + sleep_duration;
        // And then set process.setState(ProcessState::WAITING);
    }

}

Process::Process(int id, const std::vector<ProcessInstruction>& cmds)
    : m_pid(id),
      m_currentState(ProcessState::NEW),
      m_instructions(cmds),
      m_programCounter(0),
      m_creationTime(std::chrono::system_clock::now()),
      m_wakeUpTime(std::chrono::system_clock::now())
{
    std::cout << "[Process " << getPid() << "] Created. State: NEW." << std::endl;
}

void Process::setState(ProcessState newState) {
    m_currentState = newState;
}

bool Process::isFinished() const {
    return m_programCounter >= m_instructions.size() && m_currentState == ProcessState::TERMINATED;
}

void Process::executeNextInstruction(Kernel& kernel) {
    if (m_currentState != ProcessState::RUNNING) {
        std::cerr << "Error: Process " << getPid() << " attempted to execute while not RUNNING. Current State: " << static_cast<int>(m_currentState) << std::endl;
        return;
    }

    if (m_programCounter < m_instructions.size()) {
        const ProcessInstruction& current_instruction = m_instructions[m_programCounter];

        executeSingleInstruction(current_instruction, *this, kernel);

        m_programCounter++;

        if (m_programCounter >= m_instructions.size()) {
            setState(ProcessState::TERMINATED);
            std::cout << "[Process " << getPid() << "] All instructions executed. Transitioning to TERMINATED." << std::endl;
        }
    } else {
        setState(ProcessState::TERMINATED);
        std::cerr << "Warning: Process " << getPid() << " tried to execute past its last instruction. Forcing TERMINATED state." << std::endl;
    }
}
