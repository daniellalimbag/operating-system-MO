// ProcessInstruction.cpp
#include "ProcessInstruction.h"
#include "Process.h"
#include "Kernel.h"
#include <iostream>
#include <stdexcept>
#include <limits>

namespace {
    // Helper function to resolve an operand's value, which is local to this file.
    uint16_t resolveOperand(const std::string& operand, Process& process, Kernel& kernel) {
        if (process.hasVariable(operand)) {
            // Operand is a declared variable, read its value
            size_t virtualAddress = process.getVirtualAddressForVariable(operand);
            return kernel.readMemory(process, virtualAddress);
        } else if (process.isNumeric(operand)) {
            // Operand is a numeric literal, convert it to a number
            try {
                int value = std::stoi(operand);
                return process.clampUint16(value);
            } catch (const std::out_of_range& e) {
                // Handle out-of-range numbers
                std::cerr << "Process " << process.getPid() << ": Operand '" << operand << "' out of range. Terminating process." << std::endl;
                process.setState(ProcessState::TERMINATED);
                return 0;
            }
        } else {
            // Operand is an undeclared variable name, auto-declare it and initialize to 0
            process.allocateVariable(operand);
            size_t virtualAddress = process.getVirtualAddressForVariable(operand);
            kernel.handleMemoryAccess(process, virtualAddress);
            kernel.writeMemory(process, virtualAddress, 0);
            return 0;
        }
    }
}

void DeclareInstruction::execute(Process& process, Kernel& kernel) {
    process.allocateVariable(m_varName);
    size_t virtualAddress = process.getVirtualAddressForVariable(m_varName);

    kernel.handleMemoryAccess(process, virtualAddress);
    kernel.writeMemory(process, virtualAddress, m_value);
}

void AddInstruction::execute(Process& process, Kernel& kernel) {
    uint16_t val1 = resolveOperand(m_operand1, process, kernel);
    uint16_t val2 = resolveOperand(m_operand2, process, kernel);

    int result = static_cast<int>(val1) + static_cast<int>(val2);
    uint16_t clampedResult = process.clampUint16(result);

    process.allocateVariable(m_destination);
    size_t virtualAddress = process.getVirtualAddressForVariable(m_destination);
    kernel.handleMemoryAccess(process, virtualAddress);
    kernel.writeMemory(process, virtualAddress, clampedResult);
}

void SubtractInstruction::execute(Process& process, Kernel& kernel) {
    uint16_t val1 = resolveOperand(m_operand1, process, kernel);
    uint16_t val2 = resolveOperand(m_operand2, process, kernel);

    int result = static_cast<int>(val1) - static_cast<int>(val2);
    uint16_t clampedResult = process.clampUint16(result);

    process.allocateVariable(m_destination);
    size_t virtualAddress = process.getVirtualAddressForVariable(m_destination);
    kernel.handleMemoryAccess(process, virtualAddress);
    kernel.writeMemory(process, virtualAddress, clampedResult);
}

void PrintInstruction::execute(Process& process, Kernel& kernel) {
    (void)kernel;
    process.addToLog("PRINT: " + m_message);
}

void SleepInstruction::execute(Process& process, Kernel& kernel) {
    (void)kernel;
    process.setSleepTicks(m_ticksToSleep);
}
