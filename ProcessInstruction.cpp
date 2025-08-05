// ProcessInstruction.cpp
#include "ProcessInstruction.h"
#include "Process.h"
#include "Kernel.h"
#include <iostream>
#include <stdexcept>
#include <limits>
#include <cctype>

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
    std::string output = m_message;
    std::string literalPart;
    std::string varName;
    uint16_t varValue = 0;

    // Find the opening single quote
    size_t quoteStart = output.find('\'');
    if (quoteStart != std::string::npos) {
        // Find the closing single quote
        size_t quoteEnd = output.find('\'', quoteStart + 1);
        if (quoteEnd != std::string::npos) {
            literalPart = output.substr(quoteStart + 1, quoteEnd - quoteStart - 1);

            // Find the '+' operator after the string literal
            size_t plusPos = output.find('+', quoteEnd);
            if (plusPos != std::string::npos) {
                // Find the variable name after the '+'
                size_t varStart = output.find_first_not_of(" \t", plusPos + 1);
                if (varStart != std::string::npos) {
                    size_t varEnd = output.find_first_of(" \t;)", varStart);
                    if (varEnd == std::string::npos) {
                        varEnd = output.length();
                    }
                    varName = output.substr(varStart, varEnd - varStart);

                    // Now, resolve the variable's value
                    try {
                        varValue = resolveOperand(varName, process, kernel);
                        process.addToLog(literalPart + std::to_string(varValue));
                        return;
                    } catch (const std::exception& e) {
                        process.setState(ProcessState::TERMINATED);
                        return;
                    }
                }
            } else {
                // Case where there is only a literal part and no variable
                process.addToLog(literalPart);
                return;
            }
        }
    }
    // Fallback: If parsing fails for any reason, add the original message to the log
    process.addToLog(m_message);
}

void SleepInstruction::execute(Process& process, Kernel& kernel) {
    (void)kernel;
    process.setSleepTicks(m_ticksToSleep);
}
