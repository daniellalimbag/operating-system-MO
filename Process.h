#pragma once
#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector> 

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

public:
    Process() = delete;

    Process(const std::string& name, int id, int core = 1)
        : processName(name), 
          pid(id), 
          core_assigned(core),
          cpu_utilization(0),
          currentInstruction(0),
          execution_complete(false) {
        // Generate timestamp
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm;
        localtime_s(&local_tm, &now_time_t);
        std::ostringstream oss;
        oss << std::put_time(&local_tm, "%m/%d/%Y, %I:%M:%S %p");
        timestamp = oss.str();
    }

    std::string executeNextInstruction() {
        if (currentInstruction >= instructions.size()) {
            execution_complete = true;
            return "Finished!";
        }
        
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm;
        localtime_s(&local_tm, &now_time_t);
        std::ostringstream oss;
        oss << std::put_time(&local_tm, "%m/%d/%Y, %I:%M:%S %p");
        std::string currentTime = oss.str();
        
        // Get the current instruction
        std::string instruction = instructions[currentInstruction];
        std::string logEntry = "(" + currentTime + ") Core: " + 
                             std::to_string(core_assigned) + " \"" + 
                             instruction + "\"";
        
        logs.emplace_back(currentTime, logEntry);
        
        currentInstruction++;
        
        if (currentInstruction >= instructions.size()) {
            execution_complete = true;
        }
        
        return instruction;
    }

    void addInstruction(const std::string& instruction) {
        instructions.push_back(instruction);
    }

    // Display
    void viewProcess() const {
        system("cls");
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