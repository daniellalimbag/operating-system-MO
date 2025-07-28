#include "Process.h"
#include "ProcessInstruction.h"

#include <iostream>
#include <string>
#include <limits>
#include <algorithm>

Process::Process(int id, std::string processName, uint32_t memoryRequired, std::vector<std::unique_ptr<IProcessInstruction>>&& cmds)
    : m_pid(id),
      m_processName(processName),
      m_memoryRequired(memoryRequired),
      m_currentState(ProcessState::NEW),
      m_instructions(std::move(cmds)),
      m_programCounter(0UL),
      m_creationTime(std::chrono::system_clock::now()),
      m_sleepTicksRemaining(0U) {}

void Process::setState(ProcessState newState) {
    m_currentState = newState;
}

bool Process::isFinished() const {
    // A process is finished if its main program counter is at or beyond the end
    // AND its loop stack is empty (no pending FOR loops).
    return (m_programCounter >= m_instructions.size() && m_loopStack.empty());
}

void Process::setSleepTicks(uint8_t ticks) {
    if (ticks > 0) {
        m_sleepTicksRemaining = ticks;
        setState(ProcessState::WAITING);
    } else {
        m_sleepTicksRemaining = 0;
        setState(ProcessState::READY); // If ticks <= 0, ready to run immediately
    }
}

void Process::executeNextInstruction(uint32_t coreId) {
    if (m_currentState != ProcessState::RUNNING) {
        std::cerr << "Error: Process " << getPid() << " attempted to execute while not RUNNING. Current State: " << static_cast<int>(m_currentState) << "Instruction type: " << static_cast<int>(m_instructions[m_programCounter]->getType()) << " " << m_programCounter << "/" << m_instructions.size() << "\n";
        return;
    }

    m_currentExecutionCoreId = coreId;          // Store the current execution context

    /*
    std::cout << "Process " << getPid() << " executing. Current State: " << static_cast<int>(m_currentState) << "Instruction type: " << static_cast<int>(m_instructions[m_programCounter]->getType()) << " " << m_programCounter << "/" << m_instructions.size() << "\n";
    */

    if (!m_loopStack.empty()) {
        LoopContext& currentLoop = m_loopStack.back();
        const ForInstruction* forInstr = currentLoop.forInstructionPtr;

        if (currentLoop.currentInstructionIndexInBody < forInstr->getBody().size()) { // If in a loop
            const std::unique_ptr<IProcessInstruction>& current_instruction_in_loop =
                forInstr->getBody()[currentLoop.currentInstructionIndexInBody];

            current_instruction_in_loop->execute(*this);

            currentLoop.currentInstructionIndexInBody++;

            if (currentLoop.currentInstructionIndexInBody >= forInstr->getBody().size()) {
                currentLoop.currentIteration++;

                if (currentLoop.currentIteration < currentLoop.totalRepeats) {
                    currentLoop.currentInstructionIndexInBody = 0;
                } else {
                    m_loopStack.pop_back();
                    if (m_loopStack.empty()) {
                        m_programCounter++; // Increment main PC ONLY when the outermost FOR loop completes
                    }
                }
            }
        } else {    // never reaches this branch
            std::cerr << "[Process " << m_pid << "] Warning: Loop body instructions exhausted unexpectedly. Popping loop context.\n";
            m_loopStack.pop_back();
            m_programCounter++;
        }
    } else {
        if (m_programCounter < m_instructions.size()) {
            const std::unique_ptr<IProcessInstruction>& current_instruction = m_instructions[m_programCounter];

            /*
            std::cout << m_processName << ": Executing instruction type: " << static_cast<int>(current_instruction->getType()) << " Line: " << static_cast<int>(m_programCounter) << "\n";
            */

            if (current_instruction->getType() == InstructionType::FOR) {
                current_instruction->execute(*this);
                const ForInstruction* forInstr = static_cast<const ForInstruction*>(current_instruction.get());
                bool loopStarted = (!m_loopStack.empty() && m_loopStack.back().forInstructionPtr == forInstr);

                if (!loopStarted) {
                     m_programCounter++;
                }
            } else {
                current_instruction->execute(*this);
                m_programCounter++;
            }

        } else {
            setState(ProcessState::TERMINATED);
            std::cout << "[Process " << getPid() << "] All main instructions executed. Transitioning to TERMINATED.\n"; // this should not output
        }
    }

    if (m_programCounter >= m_instructions.size() && m_loopStack.empty()) {
        m_sleepTicksRemaining = 0;
        setState(ProcessState::TERMINATED);
    }
}

uint16_t Process::clampUint16(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > std::numeric_limits<uint16_t>::max()) {
        return std::numeric_limits<uint16_t>::max();
    }
    return static_cast<uint16_t>(value);
}

void Process::declareVariable(const std::string& varName, uint16_t value) {
    m_variables[varName] = value;
}

uint16_t Process::getVariableValue(const std::string& var) {
    if (m_variables.find(var) != m_variables.end()) {
        return m_variables[var];
    }
    try {
        int value = std::stoi(var);
        return clampUint16(value);
    } catch (...) {
        // If neither variable nor number, auto-declare as 0 as stated in the specs
        m_variables[var] = 0;
        return 0;
    }
}

void Process::setVariableValue(const std::string& var, uint16_t value) {
    m_variables[var] = clampUint16(value);
}

void Process::addToLog(const std::string& message) {
    m_logBuffer.push_back(message);
}

void Process::pushLoopContext(const ForInstruction* forInstr) {
    if (m_loopStack.size() >= 3) { // Max nesting level of 3
        addToLog("[Process " + std::to_string(m_pid) + "] Warning: Max loop nesting depth reached. Skipping FOR instruction.\n");
        m_programCounter++;
        return;
    }
    m_loopStack.emplace_back(forInstr->getRepeatCount(), forInstr);
}
