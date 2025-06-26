#include "ProcessInstruction.h"
#include "Process.h"
#include <vector>

// Thread-local variables for FOR
thread_local int for_nesting_level = 0;
thread_local int for_current_rep[4] = {0,0,0,0};
thread_local int for_total_rep[4] = {0,0,0,0};

void AddInstruction::execute(Process& process) {
    uint16_t val2 = process.getVariableValue(var2);
    uint16_t val3 = process.getVariableValue(var3);
    uint16_t sum = process.clampUint16(static_cast<int>(val2) + static_cast<int>(val3));
    process.setVariableValue(var1, sum);
    extern thread_local int for_nesting_level;
    extern thread_local int for_current_rep[4];
    extern thread_local int for_total_rep[4];
    std::string log_msg = "ADD: " + var1 + " = " + std::to_string(val2) + " + " + std::to_string(val3) + " = " + std::to_string(sum);
    if (for_nesting_level > 0)
        process.addToLog("FOR: [" + log_msg + "], " + std::to_string(for_current_rep[for_nesting_level]) + "/" + std::to_string(for_total_rep[for_nesting_level]));
    else
        process.addToLog(log_msg);
}
void SubtractInstruction::execute(Process& process) {
    uint16_t val2 = process.getVariableValue(var2);
    uint16_t val3 = process.getVariableValue(var3);
    int diff = static_cast<int>(val2) - static_cast<int>(val3);
    uint16_t result = process.clampUint16(diff);
    process.setVariableValue(var1, result);
    extern thread_local int for_nesting_level;
    extern thread_local int for_current_rep[4];
    extern thread_local int for_total_rep[4];
    std::string log_msg = "SUBTRACT: " + var1 + " = " + std::to_string(val2) + " - " + std::to_string(val3) + " = " + std::to_string(result);
    if (for_nesting_level > 0)
        process.addToLog("FOR: [" + log_msg + "], " + std::to_string(for_current_rep[for_nesting_level]) + "/" + std::to_string(for_total_rep[for_nesting_level]));
    else
        process.addToLog(log_msg);
}
void SleepInstruction::execute(Process& process) {
    extern thread_local int for_nesting_level;
    extern thread_local int for_current_rep[4];
    extern thread_local int for_total_rep[4];
    std::string log_msg = "SLEEP: " + std::to_string(ticks) + " ticks";
    if (for_nesting_level > 0)
        process.addToLog("FOR: [" + log_msg + "], " + std::to_string(for_current_rep[for_nesting_level]) + "/" + std::to_string(for_total_rep[for_nesting_level]));
    else
        process.addToLog(log_msg);
    process.setSleepTicks(ticks);
}

void ForInstruction::execute(Process& process) {
    if (++for_nesting_level > 3) {
        process.addToLog("FOR: Maximum nesting level exceeded");
        --for_nesting_level;
        return;
    }
    for_total_rep[for_nesting_level] = repeats;
    for (int i = 0; i < repeats; ++i) {
        for_current_rep[for_nesting_level] = i + 1;
        for (auto& instr : instructions) {
            instr->execute(process);
        }
    }
    --for_nesting_level;
}

void PrintInstruction::execute(Process& process) {
    std::string output = message;
    size_t plusPos = output.find("+");
    if (plusPos != std::string::npos && plusPos + 1 < output.size()) {
        std::string var;
        size_t i = plusPos + 1;
        if (i < output.size() && (std::isalpha(output[i]) || output[i] == '_')) {
            var += output[i++];
            while (i < output.size() && (std::isalnum(output[i]) || output[i] == '_')) {
                var += output[i++];
            }
            uint16_t val = process.getVariableValue(var);
            output.replace(plusPos, var.size() + 1, std::to_string(val));
        }
    }
    
    extern thread_local int for_nesting_level;
    extern thread_local int for_current_rep[4];
    extern thread_local int for_total_rep[4];
    std::string log_msg = "PRINT: " + output;
    
    if (for_nesting_level > 0)
        process.addToLog("FOR: [" + log_msg + "], " + std::to_string(for_current_rep[for_nesting_level]) + "/" + std::to_string(for_total_rep[for_nesting_level]));
    else
        process.addToLog(log_msg);
}

void DeclareInstruction::execute(Process& process) {
    process.declareVariable(varName, value);
    extern thread_local int for_nesting_level;
    extern thread_local int for_current_rep[4];
    extern thread_local int for_total_rep[4];
    std::string log_msg = "DECLARE: " + varName + " = " + std::to_string(value);
    if (for_nesting_level > 0)
        process.addToLog("FOR: [" + log_msg + "], " + std::to_string(for_current_rep[for_nesting_level]) + "/" + std::to_string(for_total_rep[for_nesting_level]));
    else
        process.addToLog(log_msg);
}
