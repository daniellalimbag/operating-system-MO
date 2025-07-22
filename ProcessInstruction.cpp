#include "ProcessInstruction.h"
#include "Process.h"

#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>

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

    auto now = std::chrono::system_clock::now();                                // 1. Get the current time_point from system_clock
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);         // 2. Convert to std::time_t for date/time components (seconds resolution)
    std::tm local_tm;                                                           // 3. Convert to std::tm structure for local time components (thread-safe way)
    #ifdef _WIN32
        localtime_s(&local_tm, &now_time_t); // Windows-specific, thread-safe
    #else
        localtime_r(&now_time_t, &local_tm); // POSIX (Linux/macOS) specific, thread-safe
    #endif

                                                                                // 4. Calculate the microseconds component.
    auto us_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()                                                  // First, get the duration since epoch in microseconds
    );
    auto fractional_microseconds = us_since_epoch.count() % 1000000;            // Then, subtract the whole seconds converted to microseconds to get just the fractional part. Modulo 1,000,000 to get remainder
    std::ostringstream oss;                                                     // 5. Format the output string using stringstream
    oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(6) << fractional_microseconds; // Append microseconds (zero-padded to 6 digits)
    std::string formatted_time = oss.str();

    uint32_t coreId = process.getCurrentExecutionCoreId();

    // Prepend timestamp and core ID to the output message
    std::string prefix = "  [" + formatted_time + "] [Core: " + std::to_string(coreId) + "] "; //
    output.insert(0, prefix);

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
