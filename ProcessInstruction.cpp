#include "ProcessInstruction.h"
#include "Process.h"

// --- AddInstruction Implementation ---
void AddInstruction::execute(Process& process) {
    uint16_t val2 = process.getVariableValue(var2_operand);
    uint16_t val3 = process.getVariableValue(var3_operand);
    uint16_t sum = process.clampUint16(static_cast<int>(val2) + static_cast<int>(val3));
    process.setVariableValue(var1_name, sum);
}

// --- SubtractInstruction Implementation ---
void SubtractInstruction::execute(Process& process) {
    uint16_t val2 = process.getVariableValue(var2_operand);
    uint16_t val3 = process.getVariableValue(var3_operand);
    int diff = static_cast<int>(val2) - static_cast<int>(val3);
    uint16_t result = process.clampUint16(diff);
    process.setVariableValue(var1_name, result);
}

// --- SleepInstruction Implementation ---
void SleepInstruction::execute(Process& process) {
    process.setSleepTicks(ticks);
}

// --- ForInstruction Implementation ---
// This method now only pushes the loop context onto the process's stack.
// The actual iteration and execution of the loop body is handled by Process::executeNextInstruction.
void ForInstruction::execute(Process& process) {
    // Check if exceeding nesting limit (optional, but good for robustness)
    if (process.getLoopStack().size() >= 3) { // Max nesting level is 3
        process.addToLog("Warning: Exceeded FOR loop nesting limit (3). Skipping inner loop.");
        // Process will simply advance its main program counter past this ForInstruction
        // in executeNextInstruction after this call returns.
        return;
    }

    // Push new loop context onto the process's stack
    // Pass 'this' (a pointer to this ForInstruction object) so Process can access getBody() later.
    process.pushLoopContext(this);
}

// --- PrintInstruction Implementation ---
void PrintInstruction::execute(Process& process) {
    std::string output = message;

    // Find and replace '+var' syntax for variable interpolation
    size_t pos = 0;
    while ((pos = output.find('+', pos)) != std::string::npos) {
        size_t varStart = pos + 1; // Start after the '+'

        // Ensure there's enough string left to be a variable name
        if (varStart >= output.size()) {
            pos++; // Move past the lonely '+'
            continue;
        }

        // Check if the character after '+' is valid for a variable name start (letter or underscore)
        if (std::isalpha(output[varStart]) || output[varStart] == '_') {
            size_t varEnd = varStart;
            // Continue as long as characters are alphanumeric or underscore
            while (varEnd < output.size() &&
                   (std::isalnum(output[varEnd]) || output[varEnd] == '_')) {
                varEnd++;
            }

            if (varEnd > varStart) { // Found a valid variable name
                std::string varName = output.substr(varStart, varEnd - varStart);
                uint16_t varValue = process.getVariableValue(varName); // Get value from process

                // Replace "+varName" with its string value
                output.replace(pos, varEnd - pos, std::to_string(varValue));
                pos += std::to_string(varValue).length(); // Advance position by length of replaced string
            } else {
                pos++; // Only '+' was found, no valid variable name immediately after
            }
        } else {
            pos++; // Not a variable interpolation, just a '+' character
        }
    }

    // Add the final processed string to the process's log
    process.addToLog(output);
}

// --- DeclareInstruction Implementation ---
void DeclareInstruction::execute(Process& process) {
    process.declareVariable(varName, value);
}
