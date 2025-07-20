#include "Kernel.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>

Kernel::Kernel()
    : m_nextPid(0),
      m_cpuTicks(0ULL),
      m_running(false) {}

// Core Lifecycle Methods
void Kernel::initialize() {
    std::lock_guard<std::mutex> lock(m_kernelMutex); // This lock ensures that no other thread is modifying kernel state
    m_running.store(true);
    std::cout << "Kernel: System initialization complete.\n";
}

void Kernel::shutdown() {
    std::lock_guard<std::mutex> lock(m_kernelMutex);

    std::cout << "Kernel: Shutting down all processes and background services.\n";
    m_running.store(false);

    m_processes.clear();
    std::cout << "Kernel: System shutdown complete.\n";
}

void Kernel::run() {
    while (m_running.load()) {
        {
            std::lock_guard<std::mutex> lock(m_kernelMutex);

            advanceCpuTick();
            scheduleProcesses();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

// Process Management Methods
int Kernel::createProcess(const std::vector<ProcessInstruction>& instructions) {
    std::lock_guard<std::mutex> lock(m_kernelMutex);

    int newPid = m_nextPid++;
    auto newProcess = std::make_unique<Process>(newPid, instructions);
    newProcess->setState(ProcessState::READY);
    m_processes.push_back(std::move(newProcess));

    std::cout << "[Kernel] Created new Process with PID " << newPid << ". State: READY.\n"; // Changed std::endl to \n
    return newPid;
}

int Kernel::getProcessCurrentInstructionLine(int pid) const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    Process* p = findProcessByPid(pid);
    if (p) {
        return p->getCurrentInstructionLine();
    }
    return -1; // Return -1 if PID not found or process is invalid
}

int Kernel::getProcessTotalInstructionLines(int pid) const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    Process* p = findProcessByPid(pid);
    if (p) {
        return p->getTotalInstructionLines();
    }
    return -1; // Return -1 if PID not found or process is invalid
}

// System Clock & Time Methods
unsigned long long Kernel::getCurrentCpuTick() const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    return m_cpuTicks;
}

// I/O APIs
void Kernel::print(const std::string& message) const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    std::cout << message;
}

std::string Kernel::readLine(const std::string& prompt) const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    std::string inputLine;
    std::cout << prompt;
    std::getline(std::cin, inputLine);

    return inputLine;
}

void Kernel::clearScreen() const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    const std::string ANSI_CLEAR_SCREEN = "\033[2J\033[H";
    std::cout << ANSI_CLEAR_SCREEN;
}

// Private Internal Kernel Operations
void Kernel::advanceCpuTick() {
    // This method is called from within Kernel::run, which already holds m_kernelMutex.
    m_cpuTicks++;
}

void Kernel::scheduleProcesses() {
    // This method is called from within Kernel::run, which already holds m_kernelMutex.
    // Remove terminated processes first to keep the list clean and avoid scheduling them
    m_processes.erase(
        std::remove_if(m_processes.begin(), m_processes.end(),
                       [](const std::unique_ptr<Process>& p) {
                           return p->isFinished();
                       }),
        m_processes.end());

    // Basic Round-Robin Scheduler (for now):
    // Find the first READY process and execute one instruction.
    // In a real OS, this would be more sophisticated (priorities, time slices, context switching).
    for (const auto& process_ptr : m_processes) {
        if (process_ptr->getState() == ProcessState::READY) {
            process_ptr->setState(ProcessState::RUNNING); // Mark as running
            process_ptr->executeNextInstruction(*this);   // Execute one instruction

            // If the process is not yet terminated, put it back to READY or WAITING
            if (process_ptr->getState() == ProcessState::RUNNING) { // Still running after instruction
                process_ptr->setState(ProcessState::READY); // For round-robin, put back to READY
            }
            // If it transitioned to TERMINATED or WAITING, leave it in that state.
            break; // Only execute one process per tick for simplicity
        }
    }
}

Process* Kernel::findProcessByPid(int pid) const {
    // This method is called from within a critical section (e.g., getProcessStatus, scheduleProcesses), which already holds m_kernelMutex
    for (const auto& p : m_processes) {
        if (p->getPid() == pid) {
            return p.get(); // Return raw pointer to the Process object
        }
    }
    return nullptr; // Process not found
}
