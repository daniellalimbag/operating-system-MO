#pragma once
#include <cstring>
#include <memory>
#include <vector>
class Process;

enum class InstructionType {
    ADD,
    PRINT,
    DECLARE,
    SUBTRACT,
    SLEEP,
    FOR
};

class IProcessInstruction {
public:
    virtual ~IProcessInstruction() = default;
    virtual void execute(class Process& process) = 0;
    virtual InstructionType getType() const = 0;
};

// ADD instruction: var1 = var2/value + var3/value
class AddInstruction : public IProcessInstruction {
    std::string var1, var2, var3;
public:
    AddInstruction(const std::string& v1, const std::string& v2, const std::string& v3)
        : var1(v1), var2(v2), var3(v3) {}
    void execute(class Process& process) override;
    InstructionType getType() const override { return InstructionType::ADD; }
};

// SUBTRACT instruction: var1 = var2/value - var3/value
class SubtractInstruction : public IProcessInstruction {
    std::string var1, var2, var3;
public:
    SubtractInstruction(const std::string& v1, const std::string& v2, const std::string& v3)
        : var1(v1), var2(v2), var3(v3) {}
    void execute(class Process& process) override;
    InstructionType getType() const override { return InstructionType::SUBTRACT; }
};

// SLEEP instruction
class SleepInstruction : public IProcessInstruction {
    int ticks;
public:
    SleepInstruction(int t) : ticks(t) {}
    void execute(class Process& process) override;
    InstructionType getType() const override { return InstructionType::SLEEP; }
};

// FOR instruction (can be nested up to 3 times)
class ForInstruction : public IProcessInstruction {
    std::vector<std::unique_ptr<IProcessInstruction>> instructions;
    int repeats;
public:
    ForInstruction(std::vector<std::unique_ptr<IProcessInstruction>> instrs, int reps)
        : instructions(std::move(instrs)), repeats(reps) {}
    void execute(class Process& process) override;
    InstructionType getType() const override { return InstructionType::FOR; }
    const std::vector<std::unique_ptr<IProcessInstruction>>& getBody() const { return instructions; }
    int getRepeatCount() const { return repeats; }
};

// PRINT instruction
class PrintInstruction : public IProcessInstruction {
    std::string message;
public:
    PrintInstruction(const std::string& msg) : message(msg) {}
    void execute(class Process& process) override;
    InstructionType getType() const override { return InstructionType::PRINT; }
    std::string getMessage() const { return message; }
};

// DECLARE instruction: DECLARE(var, value)
class DeclareInstruction : public IProcessInstruction {
    std::string varName;
    uint16_t value;
public:
    DeclareInstruction(const std::string& var, uint16_t val) : varName(var), value(val) {}
    InstructionType getType() const override { return InstructionType::DECLARE; }
    void execute(class Process& process) override;
};