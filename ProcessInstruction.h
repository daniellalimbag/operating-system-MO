// ProcessInstruction.h
#pragma once

#include <string>

class Process; // Forward declaration
class Kernel;  // Forward declaration

enum class InstructionType {
    DECLARE,
    ADD,
    SUBTRACT,
    PRINT,
    SLEEP,
    READ,
    WRITE
};

/**
 * @class IProcessInstruction
 * @brief Abstract base class for all process instructions.
 */
class IProcessInstruction {
public:
    virtual ~IProcessInstruction() = default;
    virtual void execute(Process& process, Kernel& kernel) = 0; // Updated signature
    virtual InstructionType getType() const = 0;
};

// Concrete Instruction Classes
class DeclareInstruction : public IProcessInstruction {
private:
    std::string m_varName;
    uint16_t m_value;

public:
    DeclareInstruction(const std::string& varName, uint16_t value) : m_varName(varName), m_value(value) {}
    void execute(Process& process, Kernel& kernel) override;
    InstructionType getType() const override { return InstructionType::DECLARE; }
};

class AddInstruction : public IProcessInstruction {
private:
    std::string m_destination;
    std::string m_operand1;
    std::string m_operand2;
public:
    AddInstruction(const std::string& destination, const std::string& operand1, const std::string& operand2)
        : m_destination(destination), m_operand1(operand1), m_operand2(operand2) {}
    void execute(Process& process, Kernel& kernel) override;
    InstructionType getType() const override { return InstructionType::ADD; }
};

class SubtractInstruction : public IProcessInstruction {
private:
    std::string m_destination;
    std::string m_operand1;
    std::string m_operand2;
public:
    SubtractInstruction(const std::string& destination, const std::string& operand1, const std::string& operand2)
        : m_destination(destination), m_operand1(operand1), m_operand2(operand2) {}
    void execute(Process& process, Kernel& kernel) override;
    InstructionType getType() const override { return InstructionType::SUBTRACT; }
};

class PrintInstruction : public IProcessInstruction {
private:
    std::string m_message;
public:
    PrintInstruction(const std::string& message) : m_message(message) {}
    void execute(Process& process, Kernel& kernel) override;
    InstructionType getType() const override { return InstructionType::PRINT; }
};

class SleepInstruction : public IProcessInstruction {
private:
    uint8_t m_ticksToSleep;
public:
    SleepInstruction(uint8_t ticks) : m_ticksToSleep(ticks) {}
    void execute(Process& process, Kernel& kernel) override;
    InstructionType getType() const override { return InstructionType::SLEEP; }
};

class ReadInstruction : public IProcessInstruction {
private:
    std::string m_varName;
    size_t m_address;

public:
    ReadInstruction(const std::string& varName, size_t address)
        : m_varName(varName), m_address(address) {}

    void execute(Process& process, Kernel& kernel) override;

    InstructionType getType() const override { return InstructionType::READ; }
};

class WriteInstruction : public IProcessInstruction {
private:
    std::string m_varName;
    size_t m_address;

public:
    WriteInstruction(size_t address, const std::string& varName)
        : m_varName(varName), m_address(address) {}

    void execute(Process& process, Kernel& kernel) override;

    InstructionType getType() const override { return InstructionType::WRITE; }
};
