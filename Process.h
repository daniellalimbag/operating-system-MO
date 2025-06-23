#pragma once
#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <stack>
#include <cstdint>

// Add a mutex for thread-safe time access
static std::mutex timeMutex;

class Process {
private:
    std::string processName;
    std::string timestamp;
    int pid;
    int core_assigned;
    int cpu_utilization;
    std::vector<std::string> instructions;
    std::vector<std::pair<std::string, std::string>> logs; // Store logs with timestamps
    size_t currentInstruction;
    bool execution_complete;
    std::unordered_map<std::string, uint16_t> variables; // Variable storage
    int sleepTicks = 0; // For SLEEP instruction
    
    struct ForLoopState {
        std::string varName;
        uint16_t endValue;
        size_t instructionIndex;
    };
    std::stack<ForLoopState> forStack; // For up to 3 nested loops
    
    // Helper: Clamp value to uint16_t range
    uint16_t clampUint16(int value) {
        if (value < 0) return 0;
        if (value > 65535) return 65535;
        return static_cast<uint16_t>(value);
    }
    // Helper: Declare variable (if not exists)
    void declareVariable(const std::string& var) {
        if (variables.find(var) == variables.end()) {
            variables[var] = 0;
        }
    }
    // Helper: Get variable value (auto-declare)
    uint16_t getVariable(const std::string& var) {
        declareVariable(var);
        return variables[var];
    }
    // Helper: Set variable value (auto-declare, clamp)
    void setVariable(const std::string& var, int value) {
        declareVariable(var);
        variables[var] = clampUint16(value);
    }

public:
    Process() = delete;

    Process(const std::string& name, int id, int core = 1)
        : processName(name),
          pid(id),
          core_assigned(core),
          cpu_utilization(0),
          currentInstruction(0),
          execution_complete(false) {
        // Generate timestamp using thread-safe approach
        std::lock_guard<std::mutex> lock(timeMutex); // Lock for thread safety
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm;
        #ifdef _WIN32
            localtime_s(&local_tm, &now_time_t); // Use localtime_s on Windows
        #else
            localtime_r(&now_time_t, &local_tm); // Use localtime_r on POSIX systems (macOS, Linux)
        #endif
        std::ostringstream oss;
        oss << std::put_time(&local_tm, "%m/%d/%Y %I:%M:%S %p");
        timestamp = oss.str();
    }

    std::string executeNextInstruction() {
        if (execution_complete) return "Finished!";
        if (sleepTicks > 0) {
            --sleepTicks;
            return "SLEEPING";
        }
        if (currentInstruction >= instructions.size()) {
            execution_complete = true;
            return "Finished!";
        }
        // Generate timestamp using thread-safe approach
        std::lock_guard<std::mutex> lock(timeMutex);
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm;
        #ifdef _WIN32
            localtime_s(&local_tm, &now_time_t);
        #else
            localtime_r(&now_time_t, &local_tm);
        #endif
        std::ostringstream oss;
        oss << std::put_time(&local_tm, "%m/%d/%Y %I:%M:%S %p");
        std::string currentTime = oss.str();

        std::string instruction = instructions[currentInstruction];
        std::string logMessage;
        bool advance = true;

        // --- Instruction Parsing ---
        auto trim = [](const std::string& s) {
            size_t start = s.find_first_not_of(" \t");
            size_t end = s.find_last_not_of(" \t");
            return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
        };
        auto split = [](const std::string& s, char delim) {
            std::vector<std::string> tokens;
            std::string token;
            std::istringstream iss(s);
            while (std::getline(iss, token, delim)) {
                tokens.push_back(token);
            }
            return tokens;
        };

        std::string inst = trim(instruction);
        if (inst.rfind("DECLARE ", 0) == 0) {
            std::string var = trim(inst.substr(8));
            declareVariable(var);
            logMessage = "Declared variable '" + var + "' = 0";
        } else if (inst.rfind("ADD ", 0) == 0) {
            // ADD var value/var2
            auto parts = split(inst.substr(4), ' ');
            if (parts.size() == 2) {
                std::string var = trim(parts[0]);
                std::string rhs = trim(parts[1]);
                int rhsVal = 0;
                if (variables.count(rhs)) rhsVal = getVariable(rhs);
                else {
                    try { rhsVal = std::stoi(rhs); } catch (...) { rhsVal = 0; }
                }
                int result = getVariable(var) + rhsVal;
                setVariable(var, result);
                logMessage = "ADD: '" + var + "' = " + std::to_string(getVariable(var));
            } else {
                logMessage = "ADD: Invalid syntax";
            }
        } else if (inst.rfind("SUBTRACT ", 0) == 0) {
            // SUBTRACT var value/var2
            auto parts = split(inst.substr(9), ' ');
            if (parts.size() == 2) {
                std::string var = trim(parts[0]);
                std::string rhs = trim(parts[1]);
                int rhsVal = 0;
                if (variables.count(rhs)) rhsVal = getVariable(rhs);
                else {
                    try { rhsVal = std::stoi(rhs); } catch (...) { rhsVal = 0; }
                }
                int result = getVariable(var) - rhsVal;
                setVariable(var, result);
                logMessage = "SUBTRACT: '" + var + "' = " + std::to_string(getVariable(var));
            } else {
                logMessage = "SUBTRACT: Invalid syntax";
            }
        } else if (inst.rfind("SLEEP ", 0) == 0) {
            // SLEEP ticks
            std::string ticksStr = trim(inst.substr(6));
            int ticks = 0;
            try { ticks = std::stoi(ticksStr); } catch (...) { ticks = 0; }
            sleepTicks = std::max(0, ticks);
            logMessage = "SLEEP for " + std::to_string(sleepTicks) + " ticks";
        } else if (inst.rfind("FOR ", 0) == 0) {
            // FOR var start end
            auto parts = split(inst.substr(4), ' ');
            if (parts.size() == 3 && forStack.size() < 3) {
                std::string var = trim(parts[0]);
                int start = 0, end = 0;
                try { start = std::stoi(parts[1]); } catch (...) { start = 0; }
                try { end = std::stoi(parts[2]); } catch (...) { end = 0; }
                setVariable(var, start);
                ForLoopState state{var, static_cast<uint16_t>(end), currentInstruction};
                forStack.push(state);
                logMessage = "FOR loop: '" + var + "' from " + std::to_string(start) + " to " + std::to_string(end); 
            } else {
                logMessage = "FOR: Invalid syntax or nesting limit";
            }
        } else if (inst == "END FOR") {
            if (!forStack.empty()) {
                auto& state = forStack.top();
                int nextVal = getVariable(state.varName) + 1;
                if (nextVal <= state.endValue) {
                    setVariable(state.varName, nextVal);
                    currentInstruction = state.instructionIndex; // Jump back to FOR
                    advance = false;
                    logMessage = "FOR loop continue: '" + state.varName + "' = " + std::to_string(nextVal);
                } else {
                    forStack.pop();
                    logMessage = "FOR loop end: '" + state.varName + "'";
                }
            } else {
                logMessage = "END FOR: No active loop";
            }
        } else if (inst.rfind("PRINT(", 0) == 0) {
            size_t start = inst.find('"');
            size_t end = inst.rfind('"');
            std::string msg;
            if (start != std::string::npos && end != std::string::npos && end > start) {
                msg = inst.substr(start + 1, end - start - 1);
            } else {
                msg = "Hello world from " + processName + "!";
            }
            logMessage = msg;
        } else if (inst == "PRINT") {
            logMessage = "Hello world from " + processName + "!";
        } else if (inst == "EXIT") {
            execution_complete = true;
            logMessage = "Process exited.";
        } else {
            logMessage = "Unknown instruction: " + inst;
        }

        std::string logEntry = "(" + currentTime + ") Core: " + std::to_string(core_assigned) + " \"" + logMessage + "\"";
        logs.emplace_back(currentTime, logEntry);
        if (advance) ++currentInstruction;
        if (currentInstruction >= instructions.size()) execution_complete = true;
        return instruction;
    }

    void addInstruction(const std::string& instruction) {
        instructions.push_back(instruction);
    }

    // Display
    void viewProcess() const {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
        std::cout << "Process Name: " << processName << " (PID: " << pid << ")\n";
        std::cout << "Core Assigned: " << std::to_string(core_assigned) << "\n";
        std::cout << "CPU Utilization: " << cpu_utilization << "%\n";
        std::cout << "Current Instruction: " << currentInstruction + 1 << " / " << instructions.size() << "\n";
        if (currentInstruction < instructions.size()) {
            std::cout << "Executing: " << instructions[currentInstruction] << "\n";
        }
        std::cout << "Created: " << timestamp << "\n";
    }

    void viewSummary() const {
        std::cout << "PID: " << pid << " | Name: " << processName
                 << " | Core: " << core_assigned
                 << " | CPU: " << cpu_utilization << "% | Created: " << timestamp << std::endl;
    }

    // Getters
    std::string getProcessName() const { return processName; }
    int getPID() const { return pid; }
    int getCore() const { return core_assigned; }
    int getCPUUtilization() const { return cpu_utilization; }
    std::string getTimestamp() const { return timestamp; }
    size_t getCurrentInstructionNumber() const { return currentInstruction + 1; }
    size_t getTotalInstructions() const { return instructions.size(); }
    bool isComplete() const { return execution_complete; }
    std::string getLogFileName() const { return processName + ".txt"; }

    std::string getLogs() const {
        std::string result;
        for (const auto& log : logs) {
            result += log.second + "\n";
        }
        return result;
    }

    // Setters
    void setCore(int core) { core_assigned = core; }
    void setCPUUtilization(int util) { cpu_utilization = util; }
};
