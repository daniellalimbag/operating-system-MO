#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

class Process;

/**
 * @enum InstructionType
 * @brief Defines the type of a process instruction.
 */
enum class InstructionType {
    ADD,
    PRINT,
    DECLARE,
    SUBTRACT,
    SLEEP,
    FOR
};

/**
 * @class IProcessInstruction
 * @brief Base interface for all process instructions.
 * @details Provides a polymorphic interface for executing instructions.
 */
class IProcessInstruction {
public:
    virtual ~IProcessInstruction() = default;

    /**
     * @brief Executes the instruction on the given process.
     * @param process The Process instance on which the instruction should operate.
     * @note If instructions also need direct Kernel services (e.g., for direct console print
     * or very specific syscalls), you might add a `Kernel& kernel` parameter here.
     * For now, it's kept concise as `Process` can relay to `Kernel` if needed.
     */
    virtual void execute(Process& process) = 0;

    /**
     * @brief Returns the type of the instruction.
     * @return An InstructionType enum value.
     */
    virtual InstructionType getType() const = 0;
};

// --- Concrete Instruction Classes ---

/**
 * @class AddInstruction
 * @brief Represents an ADD instruction: var1 = var2/value + var3/value.
 */
class AddInstruction : public IProcessInstruction {
    std::string var1_name;    // Variable to store the result
    std::string var2_operand; // Can be a variable name or a literal value
    std::string var3_operand; // Can be a variable name or a literal value
public:
    AddInstruction(const std::string& v1_name, const std::string& v2_op, const std::string& v3_op)
        : var1_name(v1_name), var2_operand(v2_op), var3_operand(v3_op) {}
    void execute(Process& process) override;
    InstructionType getType() const override { return InstructionType::ADD; }
};

/**
 * @class SubtractInstruction
 * @brief Represents a SUBTRACT instruction: var1 = var2/value - var3/value.
 */
class SubtractInstruction : public IProcessInstruction {
    std::string var1_name;
    std::string var2_operand;
    std::string var3_operand;
public:
    SubtractInstruction(const std::string& v1_name, const std::string& v2_op, const std::string& v3_op)
        : var1_name(v1_name), var2_operand(v2_op), var3_operand(v3_op) {}
    void execute(Process& process) override;
    InstructionType getType() const override { return InstructionType::SUBTRACT; }
};

/**
 * @class SleepInstruction
 * @brief Represents a SLEEP instruction, causing the process to wait for a specified number of CPU ticks.
 */
class SleepInstruction : public IProcessInstruction {
    int ticks; // Duration to sleep in CPU ticks
public:
    SleepInstruction(int t) : ticks(t) {}
    void execute(Process& process) override;
    InstructionType getType() const override { return InstructionType::SLEEP; }
};

/**
 * @class ForInstruction
 * @brief Represents a FOR loop instruction. Contains its own body of instructions and repeat count.
 * @details The execution logic for looping is managed by the Process class's `executeNextInstruction`
 * using the `LoopContext` and `m_loopStack`. This instruction primarily sets up that context.
 */
class ForInstruction : public IProcessInstruction {
    std::vector<std::unique_ptr<IProcessInstruction>> instructions_body; // The instructions within the loop body
    int repeats; // Number of times the loop body should repeat
public:
    ForInstruction(std::vector<std::unique_ptr<IProcessInstruction>> instrs, int reps)
        : instructions_body(std::move(instrs)), repeats(reps) {}
    void execute(Process& process) override; // Sets up the loop context in Process

    // Getters for Process to access loop details
    const std::vector<std::unique_ptr<IProcessInstruction>>& getBody() const { return instructions_body; }
    int getRepeatCount() const { return repeats; }
};

/**
 * @class PrintInstruction
 * @brief Represents a PRINT instruction, displaying a message to the process's log.
 */
class PrintInstruction : public IProcessInstruction {
    std::string message; // The message to print, potentially with variable interpolation
public:
    PrintInstruction(const std::string& msg) : message(msg) {}
    void execute(Process& process) override;
    InstructionType getType() const override { return InstructionType::PRINT; }
    const std::string& getMessage() const { return message; }
};

/**
 * @class DeclareInstruction
 * @brief Represents a DECLARE instruction: DECLARE(variableName, value).
 */
class DeclareInstruction : public IProcessInstruction {
    std::string varName; // Name of the variable to declare
    uint16_t value;      // Initial value of the variable
public:
    DeclareInstruction(const std::string& var, uint16_t val) : varName(var), value(val) {}
    void execute(Process& process) override;
    InstructionType getType() const override { return InstructionType::DECLARE; }
};
