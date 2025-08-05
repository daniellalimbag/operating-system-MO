// Process.cpp
#include "Process.h"
#include "ProcessInstruction.h"
#include "Kernel.h"

#include <iostream>
#include <string>
#include <limits>
#include <algorithm>
#include <stdexcept>

Process::Process(uint32_t id, std::string processName, uint32_t memoryRequired, std::vector<std::unique_ptr<IProcessInstruction>>&& cmds)
    : m_pid(id),
      m_processName(processName),
      m_memoryRequired(memoryRequired),
      m_pageTable(),
      m_variableAddresses(),
      m_nextVirtualAddressOffset(0),
      m_currentState(ProcessState::NEW),
      m_instructions(std::move(cmds)),
      m_programCounter(0UL),
      m_creationTime(std::chrono::system_clock::now()),
      m_sleepTicksRemaining(0U),
      m_logBuffer(),
      m_currentExecutionCoreId(-1) {}

void Process::setState(ProcessState newState) {
    m_currentState = newState;
}

bool Process::isFinished() const {
    return (m_programCounter >= m_instructions.size());
}

void Process::setSleepTicks(uint8_t ticks) {
    if (ticks > 0) {
        m_sleepTicksRemaining = ticks;
    } else {
        m_sleepTicksRemaining = 0;
    }
}

void Process::executeNextInstruction(uint32_t coreId, Kernel& kernel) {
    if (m_currentState != ProcessState::RUNNING) {
        std::cerr << "Error: Process " << getPid() << " attempted to execute while not RUNNING. Current State: " << static_cast<int>(m_currentState) << "Instruction type: " << static_cast<int>(m_instructions[m_programCounter]->getType()) << " " << m_programCounter << "/" << m_instructions.size() << "\n";
        return;
    }

    m_currentExecutionCoreId = coreId;

    if (m_programCounter < m_instructions.size()) {
        const std::unique_ptr<IProcessInstruction>& current_instruction = m_instructions[m_programCounter];
        current_instruction->execute(*this, kernel);
        m_programCounter++;
    }

    if (m_programCounter >= m_instructions.size()) {
        m_sleepTicksRemaining = 0;
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

void Process::allocateVariable(const std::string& varName) {
    if (m_variableAddresses.count(varName)) {
        return;
    }

    const size_t VARIABLE_SIZE = 2; // Assuming uint16_t variables
    const size_t MAX_VARIABLE_SPACE = 64;

    if (m_nextVirtualAddressOffset + VARIABLE_SIZE > MAX_VARIABLE_SPACE) {
        std::cerr << "Process " << m_pid << ": Cannot allocate variable '" << varName
                  << "'. Variable memory space exhausted.\n";
        return;
    }

    m_variableAddresses[varName] = m_nextVirtualAddressOffset;
    m_nextVirtualAddressOffset += VARIABLE_SIZE;
}

bool Process::hasVariable(const std::string& varName) const {
    return m_variableAddresses.count(varName) > 0;
}

size_t Process::getVirtualAddressForVariable(const std::string& varName) const {
    auto it = m_variableAddresses.find(varName);
    if (it != m_variableAddresses.end()) {
        return it->second;
    }
    return -1; // Return an invalid address
}

bool Process::isNumeric(const std::string& str) const {
    if (str.empty()) {
        return false;
    }
    return std::all_of(str.begin(), str.end(), [](unsigned char c){ return std::isdigit(c); });
}

void Process::addToLog(const std::string& message) {
    m_logBuffer.push_back(message);
}
