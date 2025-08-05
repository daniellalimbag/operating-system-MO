#include "Kernel.h"
#include "Process.h"
#include "SystemConfig.h"
#include "ProcessInstruction.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <random>
#include <limits>
#include <cmath>
#include <iomanip>
#include <fstream>

// ===================================================
// Anonymous helper namespace
// ===================================================

namespace {
    // in milliseconds
    const int RUN_THREAD_SLEEP_DURATION = 50;
}

// ===================================================
// Constructor
// ===================================================

Kernel::Kernel()
    : m_nextPid(0U),
      m_cpuTicks(0ULL),
      m_isInitialized(false),
      m_activeTicks(0ULL),
      m_numPagedIn(0ULL),
      m_numPagedOut(0ULL),
      m_runningGeneration(false),
      m_shutdownRequested(false) {}

// ===================================================
// Core OS Lifecycle Methods
// ===================================================

void Kernel::initialize(const SystemConfig& config) {
    {
        std::lock_guard<std::mutex> lock(m_kernelMutex);

        // Get System Configuration
        m_numCpus = config.numCPUs;
        m_schedulerType = config.scheduler;
        m_quantumCycles = config.quantumCycles;
        m_batchProcessFreq = config.batchProcessFreq;
        m_minInstructions = config.minInstructions;
        m_maxInstructions = config.maxInstructions;
        m_delaysPerExec = config.delaysPerExec;
        m_maxOverallMem = config.maxOverallMem;
        m_memPerFrame = config.memPerFrame;
        m_minMemPerProc = config.minMemPerProc;
        m_maxMemPerProc = config.maxMemPerProc;

        // Initialize Cores
        m_cpuCores.clear();
        m_cpuCores.resize(m_numCpus);
        for (uint32_t i = 0; i < m_numCpus; ++i) {
            m_cpuCores[i].id = i;
            m_cpuCores[i].currentProcess = nullptr;
            m_cpuCores[i].isBusy = false;
            m_cpuCores[i].currentQuantumTicks = 0U;
        }

        // Initialize Frames and Physical Memory
        m_totalFrames = m_maxOverallMem / m_memPerFrame;
        m_physicalMemory.resize(m_maxOverallMem / 2);
        m_frameStatus.resize(m_totalFrames, true);

        m_isInitialized.store(true);

        printHorizontalLine();
        print("Kernel: Kernel initialized with " + std::to_string(m_numCpus) + " CPU cores.\n");
        print("Kernel: Kernel initialized with " + std::to_string(m_totalFrames) + " total frames.\n");
        print("Kernel: Kernel initialized with " + std::to_string(m_maxOverallMem) + " total physical memory.\n");
        printHorizontalLine();
    }
    m_cv.notify_one();
}

void Kernel::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_kernelMutex);
        print("Kernel: Shutting down all processes and background services.\n");
        m_runningGeneration.store(false);
        m_shutdownRequested.store(true);
        m_processes.clear();
        print("Kernel: System shutdown complete.\n");
    }
    m_cv.notify_one();
}

void Kernel::run() {
    std::unique_lock<std::mutex> lock(m_kernelMutex);

    // Wait until initialized
    m_cv.wait(lock, [this]{ return m_isInitialized.load() || m_shutdownRequested.load(); });

    if (m_shutdownRequested.load()) {
        return;
    }

    // Main Kernel Loop
    while (true) {
        // Wait if nothing to do
        if (!checkIfBusy()) {
            m_cv.wait(lock);
        }

        if (m_shutdownRequested.load()) {
            break;
        }

        if (m_runningGeneration.load()) {
            if (m_cpuTicks % m_batchProcessFreq == 0) {
                generateDummyProcess("process" + std::to_string(m_nextPid), 0U);
            }
        }

        updateWaitingQueue();
        scheduleProcesses();

        if (m_cpuTicks % (m_delaysPerExec + 1ULL) == 0ULL) {
            if (executeAllCores()) {
                ++m_activeTicks;
            }
        }
        ++m_cpuTicks;

        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(RUN_THREAD_SLEEP_DURATION));
        lock.lock();
    }
}

// ===================================================
// Command APIs
// ===================================================

void Kernel::startProcessGeneration() {
    {
        std::lock_guard<std::mutex> lock(m_kernelMutex);
        if (m_runningGeneration.load()) {
            print("Kernel: Process generation is already active.\n");
            return;
        }
        m_runningGeneration.store(true);
        print("Kernel: Process generation activated.\n");
    }
    m_cv.notify_one();
}

void Kernel::stopProcessGeneration() {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    if (!m_runningGeneration.load()) {
        print("Kernel: Process generation is already inactive.\n");
        return;
    }
    m_runningGeneration.store(false);
    print("Kernel: Process generation activated.\n");
}

void Kernel::listStatus() const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    uint32_t coresBusy = 0;
    for(const auto& core: m_cpuCores) {
        if (core.isBusy) {
            ++coresBusy;
        }
    }

    print("\n");
    std::cout << "CPU Utilization: " << (static_cast<float>(coresBusy) / static_cast<float>(m_numCpus)) * 100.0f << "\%\n";
    print("Cores used: " + std::to_string(coresBusy) + "\n");
    print("Cores available: " + std::to_string(m_numCpus - coresBusy) + "\n");
    printHorizontalLine();

    if (m_processes.size() == 0) {
        print("No processes found.\n");
        printHorizontalLine();
        return;
    }

    print("Active Processes:\n");
    for (const auto& p_ptr : m_processes) {
        if(p_ptr->getState() == ProcessState::TERMINATED)
            continue;
        std::cout << "  " << p_ptr->getPname() << " (PID " << p_ptr->getPid() << ")"
                    << " (" << p_ptr->getCreationTime() << ")"
                    << " State: " << (p_ptr->getState() == ProcessState::NEW ? "NEW" :
                                        p_ptr->getState() == ProcessState::READY ? "READY" :
                                        p_ptr->getState() == ProcessState::RUNNING ? "RUNNING" :
                                        p_ptr->getState() == ProcessState::WAITING ? "WAITING" :
                                        p_ptr->getState() == ProcessState::TERMINATED ? "TERMINATED" : "UNKNOWN")
                    << " Inst: " << p_ptr->getCurrentInstructionLine()
                    << "/" << p_ptr->getTotalInstructionLines();
        if (p_ptr->getSleepTicksRemaining() > 0) {
            std::cout << " (Sleeping " << (int)p_ptr->getSleepTicksRemaining() << " ticks)";
        }
        if (p_ptr->getState() == ProcessState::RUNNING) {
            for (const auto& core : m_cpuCores) {
                if (!core.isBusy) {
                    continue;
                }
                if (p_ptr->getPid() == core.currentProcess->getPid()) {
                    std::cout << " (Core: " << core.id << ")";
                    break;
                }
            }
        }
        print("\n");
    }
    print("\n");

    print("Terminated Processes:\n");
    for (const auto& p_ptr : m_processes) {
        if(p_ptr->getState() != ProcessState::TERMINATED)
            continue;
        std::cout << "  " << p_ptr->getPname() << " (PID " << p_ptr->getPid() << ")"
                    << " (" << p_ptr->getCreationTime() << ")"
                    << " State: " << (p_ptr->getState() == ProcessState::NEW ? "NEW" :
                                        p_ptr->getState() == ProcessState::READY ? "READY" :
                                        p_ptr->getState() == ProcessState::RUNNING ? "RUNNING" :
                                        p_ptr->getState() == ProcessState::WAITING ? "WAITING" :
                                        p_ptr->getState() == ProcessState::TERMINATED ? "TERMINATED" : "UNKNOWN")
                    << " Inst: " << p_ptr->getCurrentInstructionLine()
                    << "/" << p_ptr->getTotalInstructionLines();
        if (p_ptr->getSleepTicksRemaining() > 0) {
            std::cout << " (Sleeping " << (int)p_ptr->getSleepTicksRemaining() << " ticks)";
        }
        print("\n");
    }
    printHorizontalLine();
}

void Kernel::exportListStatusToFile(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Kernel: Failed to open " << filename << " for writing.\n";
        return;
    }

    uint32_t coresBusy = 0;
    for (const auto& core : m_cpuCores) {
        if (core.isBusy) {
            ++coresBusy;
        }
    }

    outFile << "\n";
    outFile << "CPU Utilization: " << (static_cast<float>(coresBusy) / static_cast<float>(m_numCpus)) * 100.0f << "%\n";
    outFile << "Cores used: " << coresBusy << "\n";
    outFile << "Cores available: " << (m_numCpus - coresBusy) << "\n";
    outFile << "----------------------------------------\n";

    if (m_processes.empty()) {
        outFile << "No processes found.\n";
        outFile << "----------------------------------------\n";
        return;
    }

    outFile << "Active Processes:\n";
    for (const auto& p_ptr : m_processes) {
        if (p_ptr->getState() == ProcessState::TERMINATED) continue;

        outFile << "  " << p_ptr->getPname() << " (PID " << p_ptr->getPid() << ")"
                << " (" << p_ptr->getCreationTime().time_since_epoch().count() << ")"
                << " State: ";
        switch (p_ptr->getState()) {
            case ProcessState::NEW: outFile << "NEW"; break;
            case ProcessState::READY: outFile << "READY"; break;
            case ProcessState::RUNNING: outFile << "RUNNING"; break;
            case ProcessState::WAITING: outFile << "WAITING"; break;
            case ProcessState::TERMINATED: outFile << "TERMINATED"; break;
            default: outFile << "UNKNOWN"; break;
        }
        outFile << " Inst: " << p_ptr->getCurrentInstructionLine() << "/" << p_ptr->getTotalInstructionLines();
        if (p_ptr->getSleepTicksRemaining() > 0) {
            outFile << " (Sleeping " << static_cast<int>(p_ptr->getSleepTicksRemaining()) << " ticks)";
        }
        if (p_ptr->getState() == ProcessState::RUNNING) {
            for (const auto& core : m_cpuCores) {
                if (core.isBusy && core.currentProcess && p_ptr->getPid() == core.currentProcess->getPid()) {
                    outFile << " (Core: " << core.id << ")";
                    break;
                }
            }
        }
        outFile << "\n";
    }
    outFile << "\n";

    outFile << "Terminated Processes:\n";
    for (const auto& p_ptr : m_processes) {
        if (p_ptr->getState() != ProcessState::TERMINATED) continue;

        outFile << "  " << p_ptr->getPname() << " (PID " << p_ptr->getPid() << ")"
                << " (" << p_ptr->getCreationTime().time_since_epoch().count() << ")"
                << " State: TERMINATED"
                << " Inst: " << p_ptr->getCurrentInstructionLine() << "/" << p_ptr->getTotalInstructionLines();
        if (p_ptr->getSleepTicksRemaining() > 0) {
            outFile << " (Sleeping " << static_cast<int>(p_ptr->getSleepTicksRemaining()) << " ticks)";
        }
        outFile << "\n";
    }
    outFile << "----------------------------------------\n";

    outFile.close();
    print("Kernel: Process utilization report saved to " + filename + "\n");
}

Process* Kernel::reattachToProcess(const std::string& processName) const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);

    // Search for the process by name
    Process* foundProcess = nullptr;
    for (const auto& p_ptr : m_processes) {
        if (p_ptr->getPname() == processName) {
            foundProcess = p_ptr.get();
            if (p_ptr->getState() == ProcessState::TERMINATED) {
                return nullptr;
            }
            break;
        }
    }

    if(!foundProcess) {
        return nullptr;
    }

    clearScreen();
    displayProcess(foundProcess);

    return foundProcess;
}

Process* Kernel::startProcess(const std::string& processName, uint32_t memRequired) {
    Process* newProcess;
    {
        std::lock_guard<std::mutex> lock(m_kernelMutex);
        newProcess = generateDummyProcess(processName, memRequired);

        clearScreen();
        displayProcess(newProcess);
    }
    m_cv.notify_one();
    return newProcess;
}

void Kernel::printSmi(Process* process) const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    displayProcess(process);
}

void Kernel::printMemoryUtilizationReport() const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    uint32_t coresBusy = 0;
    for(const auto& core: m_cpuCores) {
        if (core.isBusy) {
            ++coresBusy;
        }
    }

    uint32_t framesOccupied = 0;
    for(const auto& p_ptr : m_processes) {
        if (p_ptr->getState() != ProcessState::TERMINATED) {
            framesOccupied += p_ptr->getPageTable().size();
        }
    }

    std::cout << "CPU Utilization: " << ((static_cast<float>(coresBusy) / static_cast<float>(m_numCpus)) * 100.0f) << "\%\n";
    std::cout << "Memory Usage: " << framesOccupied * m_memPerFrame << "B/" << m_maxOverallMem << "B\n";
    std::cout << "Memory Utilization: " << ((static_cast<float>(framesOccupied) / static_cast<float>(m_totalFrames)) * 100.0f) << "\%\n";
    std::cout << "Memory per frame: " << m_memPerFrame << "B \n";

    printHorizontalLine();

    std::vector<int> frameOccupancy(m_totalFrames, -1);
    for (const auto& p_ptr : m_processes) {
        if (p_ptr->getState() != ProcessState::TERMINATED) {
            for (const auto& pair : p_ptr->getPageTable()) {
                frameOccupancy[pair.second] = p_ptr->getPid();
            }
        }
    }

    for (uint32_t i = 0; i < m_totalFrames; ++i) {
        if (frameOccupancy[i] != -1) {
            std::cout << "Frame " << i << ": Process " << frameOccupancy[i] << "\n";
        } else {
            std::cout << "Frame " << i << ": Unoccupied\n";
        }
    }

    printHorizontalLine();
}

void Kernel::printMemoryStatistics() const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);

    uint32_t framesOccupied = 0;
    for(const auto& p_ptr : m_processes) {
        if (p_ptr->getState() != ProcessState::TERMINATED) {
            framesOccupied += p_ptr->getPageTable().size();
        }
    }
    std::cout << "Total Memory: " << m_maxOverallMem << "B\n";
    std::cout << "Used Memory: " << framesOccupied * m_memPerFrame << "B\n";
    std::cout << "Available Memory: " << m_maxOverallMem - framesOccupied * m_memPerFrame << "B\n";
    std::cout << "Memory per frame: " << m_memPerFrame << "B \n";
    std::cout << "Total CPU Ticks: " << m_cpuTicks << "\n";
    std::cout << "Active CPU Ticks: " << m_activeTicks << "\n";
    std::cout << "Idle CPU Ticks: " << m_cpuTicks - m_activeTicks << "\n";
    std::cout << "Pages swapped in: " << m_numPagedIn << "\n";
    std::cout << "Pages swapped Out: " << m_numPagedOut << "\n";
}

// ===================================================
// I/O APIs
// ===================================================

void Kernel::print(const std::string& message) const {
    std::cout << message;
}

std::string Kernel::readLine(const std::string& prompt) const {
    std::string inputLine;
    std::cout << prompt << " ";
    std::getline(std::cin, inputLine);

    return inputLine;
}

void Kernel::clearScreen() const {
    const std::string ANSI_CLEAR_SCREEN = "\033[2J\033[H";
    std::cout << ANSI_CLEAR_SCREEN;
}

void Kernel::printHorizontalLine() const {
    std::cout << "----------------------------------------\n";
}

// ===================================================
// Internal Kernel Operations
// ===================================================

void Kernel::updateWaitingQueue() {
    // Temporary list for ready processes
    std::vector<Process*> processesToMoveToReady;

    auto new_end = std::remove_if(m_waitingQueue.begin(), m_waitingQueue.end(),
                                  [&](Process* p) {
                                      p->decrementSleepTicks();
                                      if (p->getSleepTicksRemaining() == 0) {
                                          processesToMoveToReady.push_back(p);
                                          return true;
                                      }
                                      return false;
                                  });

    m_waitingQueue.erase(new_end, m_waitingQueue.end());

    for (Process* p : processesToMoveToReady) {
        if (m_schedulerType == SchedulerType::ROUND_ROBIN) {
            // If round robin, push to ready queue
            p->setState(ProcessState::READY);
            m_readyQueue.push(p);
        } else {
            // Processes don't relinquish cpu in fcfs, so no need to reschedule
            p->setState(ProcessState::RUNNING);
        }
    }
}

void Kernel::scheduleProcesses() {
    for (auto& core : m_cpuCores) {
        if (m_readyQueue.empty()) {
            return;
        }
        if (!core.isBusy) {
            Process* p_to_schedule = m_readyQueue.front();
            m_readyQueue.pop();
            core.currentProcess = p_to_schedule;
            core.isBusy = true;
            core.currentQuantumTicks = 0U;
            p_to_schedule->setState(ProcessState::RUNNING);
        }
    }
}

bool Kernel::executeAllCores() {
    bool executeSuccess = false;
    for (auto& core : m_cpuCores) {
        if (core.isBusy && core.currentProcess != nullptr) {
            Process* p = core.currentProcess;
            if (p->getState() != ProcessState::RUNNING) {
                continue;
            }

            p->executeNextInstruction(core.id, *this);
            executeSuccess = true;

            if (m_schedulerType == SchedulerType::ROUND_ROBIN) {
                core.currentQuantumTicks++;
            }
            if (p->getSleepTicksRemaining() > 0) {
                p->setState(ProcessState::WAITING);
                m_waitingQueue.push_back(p);
                if (m_schedulerType == SchedulerType::ROUND_ROBIN) {
                    core.currentProcess = nullptr;
                    core.isBusy = false;
                }
            } else if (p->isFinished()) {
                core.currentProcess = nullptr;
                core.isBusy = false;
                core.currentQuantumTicks = 0U;
                for (const auto& pair : p->getPageTable()) {
                    m_frameStatus[pair.second] = true;
                    m_numPagedOut++;
                }
                p->getPageTable().clear();
            } else if (m_schedulerType == SchedulerType::ROUND_ROBIN && core.currentQuantumTicks >= m_quantumCycles) {
                p->setState(ProcessState::READY);
                m_readyQueue.push(p);
                core.currentProcess = nullptr;
                core.isBusy = false;
                core.currentQuantumTicks = 0U;
                for (const auto& pair : p->getPageTable()) {
                    m_frameStatus[pair.second] = true;
                    m_numPagedOut++;
                }
                p->getPageTable().clear();
            }
        }
    }
    return executeSuccess;
}

bool Kernel::checkIfBusy() {
    if (m_runningGeneration.load()) {
        return true;
    }
    if (!m_readyQueue.empty() || !m_waitingQueue.empty()) {
        return true;
    }
    for (auto& core : m_cpuCores) {
        if (core.isBusy) {
            return true;
        }
    }
    return false;
}

Process* Kernel::generateDummyProcess(const std::string& newPname, uint32_t memRequired){
    static std::random_device rd;
    static std::mt19937 gen(rd());
    SystemConfig defaultConfig;

    if (m_minInstructions == 0 || m_maxInstructions == 0 || m_minInstructions > m_maxInstructions) {
        std::cerr << "[Kernel] Warning: Invalid instruction range (" << m_minInstructions
                  << "-" << m_maxInstructions << "). Using default range [1000, 2000].\n";
        m_minInstructions = defaultConfig.minInstructions;
        m_maxInstructions = defaultConfig.maxInstructions;
    }
    std::uniform_int_distribution<uint32_t> distrib_num_instructions(m_minInstructions, m_maxInstructions);
    uint32_t numInstructions = distrib_num_instructions(gen);
    std::vector<std::unique_ptr<IProcessInstruction>> instructions;
    instructions.reserve(numInstructions);

    const std::vector<std::string> varNames = {"a", "b", "c", "x", "y", "counter", "temp"};
    std::uniform_int_distribution<uint16_t> distrib_value(std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max());
    std::uniform_int_distribution<unsigned int> distrib_sleep_ticks(1, 255);
    std::uniform_int_distribution<int> distrib_loop_repeats(1, 3);

    if (memRequired == 0U) {
        if (m_minMemPerProc > m_maxMemPerProc) {
            std::cerr << "[Kernel] Warning: Invalid memory range (" << m_minMemPerProc
                  << "-" << m_maxMemPerProc << "). Using default range [64, 128].\n";
            m_minMemPerProc = defaultConfig.minMemPerProc;
            m_maxMemPerProc = defaultConfig.maxMemPerProc;
        }
        std::uniform_int_distribution<uint32_t> distrib_mem_required(m_minMemPerProc, m_maxMemPerProc);
        memRequired = distrib_mem_required(gen);
    }

    enum class DummyInstructionType {
        ADD, PRINT, DECLARE, SUBTRACT, SLEEP
    };
    std::uniform_int_distribution<> distrib_instr_type(static_cast<int>(DummyInstructionType::ADD), static_cast<int>(DummyInstructionType::SLEEP));

    for (size_t i = 0; i < numInstructions; ++i) {
        DummyInstructionType instrType = static_cast<DummyInstructionType>(distrib_instr_type(gen));

        auto getRandomVarName = [&]() {
            return varNames[std::uniform_int_distribution<>(0, varNames.size() - 1)(gen)];
        };

        auto getRandomOperand = [&]() {
            if (std::uniform_int_distribution<>(0, 1)(gen) == 0) {
                return getRandomVarName();
            } else {
                return std::to_string(distrib_value(gen));
            }
        };

        switch (instrType) {
            case DummyInstructionType::ADD:
                instructions.push_back(std::make_unique<AddInstruction>(
                    getRandomVarName(), getRandomOperand(), getRandomOperand()
                ));
                break;
            case DummyInstructionType::PRINT: {
                std::string msg = "Hello world from " + newPname + "!";
                instructions.push_back(std::make_unique<PrintInstruction>(msg));
                break;
            }
            case DummyInstructionType::DECLARE:
                instructions.push_back(std::make_unique<DeclareInstruction>(
                    getRandomVarName(), distrib_value(gen)
                ));
                break;
            case DummyInstructionType::SUBTRACT:
                instructions.push_back(std::make_unique<SubtractInstruction>(
                    getRandomVarName(), getRandomOperand(), getRandomOperand()
                ));
                break;
            case DummyInstructionType::SLEEP:
                instructions.push_back(std::make_unique<SleepInstruction>(
                    distrib_sleep_ticks(gen)
                ));
                break;
        }
    }

    uint32_t newPid = m_nextPid++;
    std::unique_ptr<Process> newProcess = std::make_unique<Process>(newPid, newPname, memRequired, std::move(instructions));
    Process* rawProcessPtr = newProcess.get();
    m_processes.push_back(std::move(newProcess));

    rawProcessPtr->setState(ProcessState::READY);
    m_readyQueue.push(rawProcessPtr);

    return rawProcessPtr;
}

void Kernel::displayProcess(Process* process) const {
    std::cout << "Process Name: " << process->getPname() << "\n";
    std::cout << "ID: " << process->getPid() << "\n";
    std::cout << "Logs:\n";
    const auto& logBuffer = process->getLogBuffer();
    if (logBuffer.empty()) {
        std::cout << "Process log is empty.\n";
    } else {
        for (const std::string& logEntry : logBuffer) {
            std::cout << logEntry << "\n";
        }
    }
    std::cout << "--- End of process log ---\n";
    std::cout << "Current instruction line: " << process->getCurrentInstructionLine() << "\n";
    std::cout << "Lines of code: " << process->getTotalInstructionLines() << "\n";
    std::cout << "Memory Required: " << process->getMemoryRequired() << "\n";
    /*
    std::cout << "\nDeclared Variables:\n";
    auto variables = process->getVariableAddresses();
    if (variables.empty()) {
        std::cout << "No variables declared.\n";
    } else {
        std::cout << std::left << std::setw(15) << "Variable" << std::setw(20) << "Virtual Address" << "Value\n";
        std::cout << "-------------------------------------------\n";
        for (const auto& var : variables) {
            uint16_t value = readMemory(*process, var.second);
            std::cout << std::left << std::setw(15) << var.first
                      << std::setw(20) << var.second << "\n";
        }
    }
    */
}

ssize_t Kernel::findFreeFrame() const {
    for (size_t i = 0; i < m_frameStatus.size(); ++i) {
        if (m_frameStatus[i]) {
            return i;
        }
    }
    return -1;
}

// ===================================================
// Public Memory Access APIs
// ===================================================

/**
 * @brief Handles a memory access request by ensuring the required virtual page is in a physical frame.
 * @details This function checks for a page fault and, if one occurs, finds a free frame and
 * updates the process's page table.
 * @param process The process making the request.
 * @param virtualAddress The virtual address being accessed.
 */
void Kernel::handleMemoryAccess(Process& process, size_t virtualAddress) {
    size_t virtualPageNumber = virtualAddress / m_memPerFrame;

    // Check if the virtual page is already mapped to a physical frame
    if (process.getPageTable().find(virtualPageNumber) == process.getPageTable().end()) {
        //std::cerr << "Kernel: Page fault for PID " << process.getPid() << ", virtual page " << virtualPageNumber << ".\n";
        ssize_t freeFrameIndex = findFreeFrame();

        if (freeFrameIndex != -1) {
            m_frameStatus[freeFrameIndex] = false; // Mark frame as occupied
            process.getPageTable()[virtualPageNumber] = freeFrameIndex; // Map virtual page to physical frame
            m_numPagedIn++;
        } else {
            // In a real OS, you'd use a page replacement algorithm here.
            std::cerr << "Kernel: Out of physical memory. Cannot handle page fault for PID " << process.getPid() << ".\n";
            // For now, let's just terminate the process for simplicity.
            process.setState(ProcessState::TERMINATED);
        }
    }
}

/**
 * @brief Reads a 16-bit value from a process's virtual address.
 * @details This function first ensures the page is resident, then translates the address
 * and reads from the physical memory array.
 * @param process The process making the request.
 * @param virtualAddress The virtual address to read from.
 * @return The 16-bit value read from memory.
 */
uint16_t Kernel::readMemory(Process& process, size_t virtualAddress) {
    handleMemoryAccess(process, virtualAddress);
    if (process.getState() == ProcessState::TERMINATED) {
        return 0; // Return a safe value if the process was terminated
    }

    size_t virtualPageNumber = virtualAddress / m_memPerFrame;
    size_t physicalFrameNumber = process.getPageTable()[virtualPageNumber];
    size_t offset = (virtualAddress % m_memPerFrame) / 2; // Divide by 2 because physical memory stores uint16_t
    size_t physicalAddress = (physicalFrameNumber * (m_memPerFrame / 2)) + offset;

    if (physicalAddress >= m_physicalMemory.size()) {
        std::cerr << "Kernel: Illegal memory access. Calculated physical address " << physicalAddress << " is out of bounds.\n";
        return 0;
    }

    return m_physicalMemory[physicalAddress];
}

/**
 * @brief Writes a 16-bit value to a process's virtual address.
 * @details This function first ensures the page is resident, then translates the address
 * and writes to the physical memory array.
 * @param process The process making the request.
 * @param virtualAddress The virtual address to write to.
 * @param data The 16-bit value to write.
 */
void Kernel::writeMemory(Process& process, size_t virtualAddress, uint16_t data) {
    handleMemoryAccess(process, virtualAddress);
    if (process.getState() == ProcessState::TERMINATED) {
        return; // Do nothing if the process was terminated
    }

    size_t virtualPageNumber = virtualAddress / m_memPerFrame;
    size_t physicalFrameNumber = process.getPageTable()[virtualPageNumber];
    size_t offset = (virtualAddress % m_memPerFrame) / 2;
    size_t physicalAddress = (physicalFrameNumber * (m_memPerFrame / 2)) + offset;

    if (physicalAddress >= m_physicalMemory.size()) {
        std::cerr << "Kernel: Illegal memory write. Calculated physical address " << physicalAddress << " is out of bounds.\n";
        return;
    }

    m_physicalMemory[physicalAddress] = data;
}
