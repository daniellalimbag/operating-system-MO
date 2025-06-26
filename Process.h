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
#include <memory>
#include "ProcessInstruction.h"

static std::mutex timeMutex;

struct ForLoopState {
    std::string varName;
    uint16_t endValue;
    size_t instructionIndex;
};

class Process {
private:
    std::string processName;
    std::string timestamp;
    int pid;
    int core_assigned;
    int cpu_utilization;
    
    // Instruction system stuff
    std::vector<std::unique_ptr<IProcessInstruction>> instructionList;
    size_t instructionCounter;
    bool execution_complete;
    
    // Process state management
    std::unordered_map<std::string, uint16_t> variables;
    std::vector<std::pair<std::string, std::string>> logs;
    int sleepTicks = 0;
    std::stack<ForLoopState> forStack;
    
public:
    // Helper method
    uint16_t clampUint16(int value) {
        if (value < 0) return 0;
        if (value > 65535) return 65535;
        return static_cast<uint16_t>(value);
    }
private:
    
    std::string getCurrentTimestamp() {
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
        return oss.str();
    }

public:
    Process() = delete;

    Process(const std::string& name, int id, int core = -1)
        : processName(name),
          pid(id),
          core_assigned(core),
          cpu_utilization(0),
          instructionCounter(0),
          execution_complete(false) {
        timestamp = getCurrentTimestamp();
    }

    std::string executeNextInstruction() {
        if (execution_complete) return "Finished!";
        // Sleep handlingg
        if (sleepTicks > 0) {
            --sleepTicks;
            return "SLEEPING";
        }
        if (instructionCounter >= instructionList.size()) {
            execution_complete = true;
            return "Finished!";
        }
        executeCurrentInstruction();
        moveToNextLine();
        if (instructionCounter >= instructionList.size()) {
            execution_complete = true;
        }
        return instructionList[instructionCounter - 1]->getType() == InstructionType::PRINT ? "PRINT" : "EXECUTED";
    }
    
    void executeCurrentInstruction() {
        if (instructionCounter < instructionList.size()) {
            instructionList[instructionCounter]->execute(*this);
        }
    }
    
    void moveToNextLine() {
        instructionCounter++;
    }

    void addInstruction(std::unique_ptr<IProcessInstruction> instruction) {
        instructionList.push_back(std::move(instruction));
    }

    void declareVariable(const std::string& var, uint16_t value = 0) {
        variables[var] = value;
    }
    
    uint16_t getVariableValue(const std::string& var) {
        if (variables.find(var) != variables.end()) {
            return variables[var];
        }
        try {
            int value = std::stoi(var);
            return clampUint16(value);
        } catch (...) {
            // If neither variable nor number, auto-declare as 0 as stated in the specs
            variables[var] = 0;
            return 0;
        }
    }
    
    void setVariableValue(const std::string& var, uint16_t value) {
        variables[var] = clampUint16(value);
    }

    void pushForLoop(const std::string& varName, uint16_t endValue, size_t instructionIndex) {
        if (forStack.size() < 3) { // Max 3 nested loops
            forStack.push({varName, endValue, instructionIndex});
        }
    }
    
    void popForLoop() {
        if (!forStack.empty()) {
            forStack.pop();
        }
    }
    
    bool hasActiveForLoop() const {
        return !forStack.empty();
    }
    
    ForLoopState getTopForLoop() const {
        return forStack.top();
    }

    void setSleepTicks(int ticks) {
        sleepTicks = std::max(0, ticks);
    }
    
    void setComplete(bool complete) {
        execution_complete = complete;
    }
    
    void setInstructionIndex(size_t index) {
        instructionCounter = index;
    }
    
    // Logging
    void addToLog(const std::string& message) {
        std::string currentTime = getCurrentTimestamp();
        std::string logEntry = "(" + currentTime + ") Core: " + std::to_string(core_assigned) + " \"" + message + "\"";
        logs.emplace_back(currentTime, logEntry);
    }

    // Display methods
    void viewProcess() const {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
        std::cout << "Process Name: " << processName << " (PID: " << pid << ")\n";
        std::cout << "Core Assigned: " << std::to_string(core_assigned) << "\n";
        std::cout << "CPU Utilization: " << cpu_utilization << "%\n";
        std::cout << "Current Instruction: " << instructionCounter + 1 << " / " << instructionList.size() << "\n";
        if (instructionCounter < instructionList.size()) {
            std::cout << "Executing instruction type: " << static_cast<int>(instructionList[instructionCounter]->getType()) << "\n";
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
    size_t getCurrentInstructionNumber() const { return instructionCounter + 1; }
    size_t getTotalInstructions() const { return instructionList.size(); }
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