#include "Process.h"
#include "ProcessInstruction.h"

#include <iostream>
#include <string>
#include <limits>
#include <algorithm>

Process::Process(int id, std::vector<std::unique_ptr<IProcessInstruction>>&& cmds)
    : m_pid(id),
      m_currentState(ProcessState::NEW),
      m_instructions(std::move(cmds)),
      m_programCounter(0UL),
      m_creationTime(std::chrono::system_clock::now()),
      m_sleepTicksRemaining(0U)
{
    std::cout << "[Process " << getPid() << "] Created. State: NEW. Instruction Count: " << m_instructions.size() << "\n";
}

void Process::setState(ProcessState newState) {
    m_currentState = newState;
}

bool Process::isFinished() const {
    // A process is finished if its main program counter is at or beyond the end
    // AND its loop stack is empty (no pending FOR loops).
    return (m_programCounter >= m_instructions.size() && m_loopStack.empty() && m_currentState == ProcessState::TERMINATED);
}

void Process::setSleepTicks(uint8_t ticks) {
    if (ticks > 0) {
        m_sleepTicksRemaining = ticks;
        setState(ProcessState::WAITING);
    } else {
        setState(ProcessState::READY); // If ticks <= 0, ready to run immediately
    }
}

void Process::executeNextInstruction() {
    if (m_currentState != ProcessState::RUNNING) {
        std::cerr << "Error: Process " << getPid() << " attempted to execute while not RUNNING. Current State: " << static_cast<int>(m_currentState) << "\n";
        return;
    }

    if (!m_loopStack.empty()) {
        LoopContext& currentLoop = m_loopStack.back();
        const ForInstruction* forInstr = currentLoop.forInstructionPtr;

        if (currentLoop.currentInstructionIndexInBody < forInstr->getBody().size()) {
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
                    m_programCounter++;
                }
            }
        } else {
            std::cerr << "[Process " << m_pid << "] Warning: Loop body instructions exhausted unexpectedly. Popping loop context.\n";
            m_loopStack.pop_back();
            m_programCounter++;
        }
    } else {
        if (m_programCounter < m_instructions.size()) {
            const std::unique_ptr<IProcessInstruction>& current_instruction = m_instructions[m_programCounter];

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
            std::cout << "[Process " << getPid() << "] All main instructions executed. Transitioning to TERMINATED.\n";
        }
    }

    if (m_programCounter >= m_instructions.size() && m_loopStack.empty()) {
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

size_t Process::getCurrentInstructionLine() const {
    if (!m_loopStack.empty()) {
        return m_loopStack.back().currentInstructionIndexInBody;
    }
    return m_programCounter;
}

size_t Process::getTotalInstructionLines() const {
    if (!m_loopStack.empty()) {
        return m_loopStack.back().forInstructionPtr->getBody().size();
    }
    return m_instructions.size();
}
