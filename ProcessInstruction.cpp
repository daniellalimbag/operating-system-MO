#include "ProcessInstruction.h"
#include "Process.h"
#include <vector>
#include <regex>

// Thread-local variables for FOR
thread_local int for_nesting_level = 0;
thread_local int for_current_rep[4] = {0,0,0,0};
thread_local int for_total_rep[4] = {0,0,0,0};

void AddInstruction::execute(Process& process) {
    uint16_t val2 = process.getVariableValue(var2);
    uint16_t val3 = process.getVariableValue(var3);
    uint16_t sum = process.clampUint16(static_cast<int>(val2) + static_cast<int>(val3));
    process.setVariableValue(var1, sum);
}

void SubtractInstruction::execute(Process& process) {
    uint16_t val2 = process.getVariableValue(var2);
    uint16_t val3 = process.getVariableValue(var3);
    int diff = static_cast<int>(val2) - static_cast<int>(val3);
    uint16_t result = process.clampUint16(diff);
    process.setVariableValue(var1, result);
}

void SleepInstruction::execute(Process& process) {
    process.setSleepTicks(ticks);
}

void ForInstruction::execute(Process& process) {
    if (++for_nesting_level > 3) {
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
    
    // Handle variable interpolation with +var syntax
    size_t pos = 0;
    while ((pos = output.find('+', pos)) != std::string::npos) {
        // Check if there's a variable name after the +
        size_t varStart = pos + 1;
        if (varStart >= output.size()) {
            pos++;
            continue;
        }
        
        size_t varEnd = varStart;
        if (std::isalpha(output[varStart]) || output[varStart] == '_') {
            while (varEnd < output.size() && 
                   (std::isalnum(output[varEnd]) || output[varEnd] == '_')) {
                varEnd++;
            }
            
            if (varEnd > varStart) {
                std::string varName = output.substr(varStart, varEnd - varStart);
                uint16_t varValue = process.getVariableValue(varName);
                
                output.replace(pos, varEnd - pos, std::to_string(varValue));
                pos += std::to_string(varValue).length();
            } else {
                pos++;
            }
        } else {
            pos++;
        }
    }
    
    // Add to log
    process.addToLog(output);
}

void DeclareInstruction::execute(Process& process) {
    process.declareVariable(varName, value);
}