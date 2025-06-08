#pragma once
#include <unordered_map>
#include <vector>
#include "Process.h"

class ProcessManager {
private:
    std::unordered_map<int, Process> processes;
    int next_pid;

public:
    ProcessManager() : next_pid(1) {}

    int createProcess(const std::string& name, int core = -1) {
        int pid = next_pid++;
        processes.emplace(pid, Process(name, pid, core));
        return pid;
    }

    Process* getProcess(int pid) {
        auto it = processes.find(pid);
        if (it != processes.end()) {
            return &(it->second);
        }
        return nullptr;
    }

    std::vector<const Process*> getAllProcesses() const {
        std::vector<const Process*> process_list;
        process_list.reserve(processes.size());
        for (const auto& pair : processes) {
            process_list.push_back(&pair.second);
        }
        return process_list;
    }

    size_t getProcessCount() const {
        return processes.size();
    }

    bool updateProcessUtilization(int pid, int utilization) {
        auto process = getProcess(pid);
        if (process) {
            process->setCPUUtilization(utilization);
            return true;
        }
        return false;
    }

    bool assignProcessToCore(int pid, int core) {
        auto process = getProcess(pid);
        if (process) {
            process->setCore(core);
            return true;
        }
        return false;
    }

    std::string executeProcessInstruction(int pid) {
        auto process = getProcess(pid);
        if (process) {
            return process->executeNextInstruction();
        }
        return "Process not found";
    }
}; 