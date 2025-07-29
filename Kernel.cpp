#include "Kernel.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <random>
#include <limits>
#include <cmath>

Kernel::Kernel()
    : m_nextPid(1U),
      m_cpuTicks(0ULL),
      m_isInitialized(false),
      m_runningGeneration(false),
      m_shutdownRequested(false),
      m_numCpus(4U),
      m_schedulerType(SchedulerType::ROUND_ROBIN),
      m_quantumCycles(5U),
      m_batchProcessFreq(1U),
      m_minInstructions(1000U),
      m_maxInstructions(2000U),
      m_delaysPerExec(0U),
      m_maxOverallMem(128U),
      m_memPerFrame(64U),
      m_minMemPerProc(64U),
      m_maxMemPerProc(64U) {}

// Core Lifecycle Methods
void Kernel::initialize(const SystemConfig& config) {
    {
        std::lock_guard<std::mutex> lock(m_kernelMutex); // This lock ensures that no other thread is modifying kernel state
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

        // --- Initialize CPU Cores ---
        m_cpuCores.clear(); // Clear any existing cores
        m_cpuCores.resize(m_numCpus); // Create the specified number of cores
        for (uint32_t i = 0; i < m_numCpus; ++i) {
            m_cpuCores[i].id = i;
            m_cpuCores[i].currentProcess = nullptr; // No process initially assigned
            m_cpuCores[i].isBusy = false;
            m_cpuCores[i].currentQuantumTicks = 0U;
        }
        std::cout << "Kernel: Kernel initialized with " << m_numCpus << " CPU cores.\n";
        /*
        for (const auto& core: m_cpuCores) {
            std::cout << "Core " << core.id << "\n";
        }
        */
        m_isInitialized.store(true); // Set the flag to true

        // --- Initialize Page Table ---
        m_totalFrames = m_maxOverallMem / m_memPerFrame;
        m_pageTable.resize(m_totalFrames);
        std::cout << "Kernel: Page Table initialized with " << m_totalFrames << " total frames.\n";
    }

    m_cv.notify_one();           // Notify the run() thread that initialization is complete
}

void Kernel::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_kernelMutex);
        std::cout << "Kernel: Shutting down all processes and background services.\n";
        m_runningGeneration.store(false);   // Stop new process generation
        m_shutdownRequested.store(true);    // Signal that shutdown has been requested
        m_processes.clear();
        std::cout << "Kernel: System shutdown complete.\n";
    }
    m_cv.notify_one(); // Wake up run() thread if it's waiting
}

void Kernel::run() {
    std::unique_lock<std::mutex> lock(m_kernelMutex);

    m_cv.wait(lock, [this]{ return m_isInitialized.load() || m_shutdownRequested.load(); });        // Wait here until the kernel is initialized

    if (m_shutdownRequested.load()) {
        return;
    }

    while (true) {
        if (!isBusy() && !m_runningGeneration.load()) {     // If nothing to do, then wait.
            //std::cout << "Kernel: waiting...\n";
            m_cv.wait(lock);                                // Atomically releases the lock and waits. Reacquires lock on wake-up.
        }

        if (m_shutdownRequested.load()) {      // IMMEDIATE TERMINATION: If m_shutdownRequested is true, break out of the loop.
            break;
        }

        if (m_runningGeneration.load()) {                                           // m_runningGeneration process generation
            if (m_cpuTicks % m_batchProcessFreq == 0) {                             // Process Generation Logic: Only generate if m_running is true
                std::string newPname = "process" + std::to_string(m_nextPid);
                Process* newProcess = generateDummyProcess(newPname);
                newProcess->setState(ProcessState::READY);
                m_readyQueue.push(newProcess);
            }
        }

        updateWaitingProcesses();                                                   // Update status of waiting processes (e.g., sleeping)
        scheduleProcesses();                                                        // CPU Process Scheduling
        if (m_cpuTicks % (m_delaysPerExec + 1ULL) == 0ULL) {                        // Cpu Core Process Execution
            for (auto& core : m_cpuCores) {
                if (core.isBusy && core.currentProcess != nullptr) {
                    Process* p = core.currentProcess;
                    uint32_t framePointer;
                    uint32_t pagesRequired = static_cast<uint32_t>(std::ceilf(static_cast<float>(p->getMemoryRequired()) / static_cast<float>(m_memPerFrame)));
                    for (framePointer = 0; framePointer < m_totalFrames; ++framePointer) {
                        if (m_pageTable[framePointer] == p->getPid()) {
                            break;
                        }
                    }
                    if (framePointer == m_totalFrames) {
                        // refactor it to this:
                        // framePointer = doPageFaultHandling(pagesRequired);
                        // if (framePointer == m_totalFrames) continue;                 // if couldn't find victim frame, don't execute
                        uint32_t pageCounter = 0;
                        for (framePointer = 0; framePointer < m_totalFrames; ++framePointer) {
                            if (m_pageTable[framePointer] == 0) {
                                pageCounter++;
                                if (pageCounter >= pagesRequired) {
                                    break;
                                }
                                continue;
                            } else {
                                pageCounter = 0;
                            }
                        }
                        if (framePointer == m_totalFrames && pageCounter < pagesRequired) {
                            std::cout << "[Tick " << m_cpuTicks << "] [Core " << core.id << "] " << core.currentProcess->getPname() << " could not be executed\n";
                            continue;
                        } else {
                            while (pageCounter > 0) {
                                m_pageTable[framePointer] = p->getPid();
                                --framePointer;
                                --pageCounter;
                            }
                            ++framePointer;
                            //std::cout << "Page fault handling success, Frame Pointer at " << framePointer << "\n";
                        }
                    }
                    p->executeNextInstruction(core.id);
                    std::cout << "[Tick " << m_cpuTicks << "] [Core " << core.id << "] " << core.currentProcess->getPname() << " executed at base frame " << framePointer << "\n";
                    if (m_schedulerType == SchedulerType::ROUND_ROBIN) {
                        core.currentQuantumTicks++;                                 // Increment quantum only for Round Robin
                    }
                    if (p->getState() == ProcessState::WAITING) {
                        p->setState(ProcessState::WAITING);
                        m_waitingQueue.push_back(p);                            // Process is pushed back to waiting queue
                        core.currentProcess = nullptr;                              // Core is now free
                        core.isBusy = false;
                        for (uint32_t pageCounter = 0; pageCounter < pagesRequired; ++pageCounter) {
                            m_pageTable[framePointer++] = 0;
                        }
                    } else if (p->isFinished()) {                                   // Checks if process completed all of its instructions
                        core.currentProcess = nullptr;
                        core.isBusy = false;
                        core.currentQuantumTicks = 0U;
                        for (uint32_t pageCounter = 0; pageCounter < pagesRequired; ++pageCounter) {
                            m_pageTable[framePointer++] = 0;
                        }
                    } else if (m_schedulerType == SchedulerType::ROUND_ROBIN && core.currentQuantumTicks >= m_quantumCycles) {
                        // Quantum expired for this process in Round Robin mode
                        p->setState(ProcessState::READY);                           // Preempt process
                        m_readyQueue.push(p);
                        core.currentProcess = nullptr;                              // Free the core
                        core.isBusy = false;
                        core.currentQuantumTicks = 0U;
                        for (uint32_t pageCounter = 0; pageCounter < pagesRequired; ++pageCounter) {
                            m_pageTable[framePointer++] = 0;
                        }
                    }
                }
            }
        }
        m_cpuTicks++;

        lock.unlock(); // Release the lock before sleeping

        // Small sleep to prevent busy-waiting.
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        lock.lock();   // Re-acquire the lock for the next iteration
    }
}

// Dummy Process Generator
Process* Kernel::generateDummyProcess(const std::string& newPname) {
    // No need for mutex because it's only called within run() and startProcess()
    // Use a random number generator
    // static ensures the generator is initialized only once per program run
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // Determine random number of instructions within the specified range
    // Ensure m_minInstructions and m_maxInstructions are initialized (e.g., in Kernel constructor or initialize method)
    if (m_minInstructions == 0 || m_maxInstructions == 0 || m_minInstructions > m_maxInstructions) {
        std::cerr << "[Kernel] Warning: Invalid instruction range (" << m_minInstructions
                  << "-" << m_maxInstructions << "). Using default range [10, 50].\n";
        m_minInstructions = 1000U; // Fallback to default
        m_maxInstructions = 2000U;
    }
    std::uniform_int_distribution<uint32_t> distrib_num_instructions(m_minInstructions, m_maxInstructions);
    uint32_t numInstructions = distrib_num_instructions(gen);

    std::vector<std::unique_ptr<IProcessInstruction>> instructions;
    instructions.reserve(numInstructions);

    // A list of possible variable names to use in dummy processes
    const std::vector<std::string> varNames = {"a", "b", "c", "x", "y", "counter", "temp"};
    // A distribution for random values (0 to 65,535 for uint16_t)
    std::uniform_int_distribution<uint16_t> distrib_value(std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max());
    // A distribution for sleep ticks (1 to 255 ticks for uint8_t)
    std::uniform_int_distribution<uint8_t> distrib_sleep_ticks(1, 255);
    // A distribution for loop repeats (e.g., 1 to 3 repeats)
    std::uniform_int_distribution<int> distrib_loop_repeats(1, 3);
    // A distribution for memory required (e.g., 64 to 65,536 for uint32_t)
    std::uniform_int_distribution<uint32_t> distrib_mem_required(m_minMemPerProc, m_maxMemPerProc);
    uint32_t memRequired = distrib_mem_required(gen);

    // Define the types of instructions we can generate
    enum class DummyInstructionType {
        ADD, SUBTRACT, SLEEP, FOR, PRINT, DECLARE
    };
    // A distribution for instruction types (e.g., ADD to DECLARE)
    std::uniform_int_distribution<> distrib_instr_type(static_cast<int>(DummyInstructionType::ADD), static_cast<int>(DummyInstructionType::DECLARE));

    for (size_t i = 0; i < numInstructions; ++i) {
        DummyInstructionType instrType = static_cast<DummyInstructionType>(distrib_instr_type(gen));

        // Helper to get a random variable name
        auto getRandomVarName = [&]() {
            return varNames[std::uniform_int_distribution<>(0, varNames.size() - 1)(gen)];
        };

        // Helper to get a random variable name or a literal value string
        auto getRandomOperand = [&]() {
            if (std::uniform_int_distribution<>(0, 1)(gen) == 0) { // 50% chance for variable
                return getRandomVarName();
            } else { // 50% chance for a literal number
                return std::to_string(distrib_value(gen));
            }
        };

        // Avoid too many nested loops immediately, or loops that are too long in small processes
        bool canCreateForLoop = (instructions.size() < numInstructions / 2) && (numInstructions - instructions.size() > 5);

        switch (instrType) {
            case DummyInstructionType::ADD:
                instructions.push_back(std::make_unique<AddInstruction>(
                    getRandomVarName(), getRandomOperand(), getRandomOperand()
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
            case DummyInstructionType::FOR:
                // Only create FOR loops if there's enough room for a body and it won't exceed nesting limit
                if (canCreateForLoop) {
                    // Create a small, simple loop body
                    std::vector<std::unique_ptr<IProcessInstruction>> loopBody;
                    int bodySize = std::uniform_int_distribution<>(1, 3)(gen); // 1 to 3 instructions in body
                    for (int j = 0; j < bodySize; ++j) {
                        // For simplicity, mostly print or declare within loops
                        if (std::uniform_int_distribution<>(0, 1)(gen) == 0) {
                            loopBody.push_back(std::make_unique<PrintInstruction>("Hello world from " + newPname + "!"));
                        } else {
                            loopBody.push_back(std::make_unique<DeclareInstruction>(getRandomVarName(), distrib_value(gen)));
                        }
                    }
                    instructions.push_back(std::make_unique<ForInstruction>(
                        std::move(loopBody), distrib_loop_repeats(gen)
                    ));
                    break;
                }
                // If we can't create a FOR loop, fall through the PRINT case instead.
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
        }
    }

    // Create the process using std::make_unique
    uint32_t newPid = m_nextPid++;
    std::unique_ptr<Process> newProcess = std::make_unique<Process>(newPid, newPname, memRequired, std::move(instructions));

    // Get a raw pointer to the process before moving ownership to m_processes
    // This pointer will remain valid as long as the unique_ptr in m_processes manages the object.
    Process* rawProcessPtr = newProcess.get();

    // Add the unique_ptr to the kernel's process list, transferring ownership
    m_processes.push_back(std::move(newProcess));

    /*
    std::cout << "[Kernel] Generated dummy process with PID: " << newPid
              << ". Instructions: " << numInstructions
              << ". Total processes: " << m_processes.size() << "\n";
    */

    return rawProcessPtr;
}

// Process Generation Control
void Kernel::startProcessGeneration() {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    if (!m_runningGeneration.load()) {
        m_runningGeneration.store(true);
        std::cout << "Kernel: Process generation activated.\n";
        m_cv.notify_one(); // Wake up run() thread if it's waiting
    } else {
        std::cout << "Kernel: Process generation is already active.\n";
    }
}

void Kernel::stopProcessGeneration() {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    if (m_runningGeneration.load()) {
        m_runningGeneration.store(false);
        std::cout << "Kernel: Process generation deactivated.\n";
    } else {
        std::cout << "Kernel: Process generation is already inactive.\n";
    }
}

// Screen Commands
void Kernel::listStatus() const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    uint32_t coresBusy = 0;
    for(const auto& core: m_cpuCores) {
        if (core.isBusy) {
            ++coresBusy;
        }
    }
    std::cout << "CPU Utilization: " << ((static_cast<float>(coresBusy) / static_cast<float>(m_numCpus)) * 100.0f) << "\%\n";
    std::cout << "Cores used: " << coresBusy << "\n";
    std::cout << "Cores available: " << m_numCpus - coresBusy << "\n";

    std::cout << "\n----------------------------------------\n";

    if (m_processes.size() == 0) {
        std::cout << "No processes found." << "\n";
        std::cout << "----------------------------------------\n";
        return;
    }

    std::cout << "Active Processes:\n";
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
        std::cout << "\n";
    }
    std::cout << "\n";
    std::cout << "Terminated Processes:\n";
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
        std::cout << "\n";
    }
    std::cout << "----------------------------------------\n";
}

Process* Kernel::reattachToProcess(const std::string& processName) const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);

    Process* foundProcess = nullptr;                // Search for the process by name
    for (const auto& p_ptr : m_processes) {
        if (p_ptr->getPname() == processName) {
            foundProcess = p_ptr.get();
            if (p_ptr->getState() == ProcessState::TERMINATED) {    // Can't view terminated process
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

Process* Kernel::startProcess(const std::string& processName) {
    Process* newProcess;
    {
        std::lock_guard<std::mutex> lock(m_kernelMutex);
        newProcess = generateDummyProcess(processName);
        newProcess->setState(ProcessState::READY);
        m_readyQueue.push(newProcess);

        clearScreen();
        displayProcess(newProcess);
    }
    m_cv.notify_one(); // Wake up run() thread if it's waiting
    return newProcess;
}

void Kernel::printSmi(Process* process) const {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    displayProcess(process);
}

// I/O APIs
void Kernel::print(const std::string& message) const {
    std::cout << message;
}

std::string Kernel::readLine(const std::string& prompt) const {
    std::string inputLine;
    std::cout << prompt;
    std::getline(std::cin, inputLine);

    return inputLine;
}

void Kernel::clearScreen() const {
    const std::string ANSI_CLEAR_SCREEN = "\033[2J\033[H";
    std::cout << ANSI_CLEAR_SCREEN;
}

void Kernel::scheduleProcesses() {
    if (m_schedulerType == SchedulerType::FCFS || m_schedulerType == SchedulerType::ROUND_ROBIN) {
        for (auto& core : m_cpuCores) {                             // Iterate in order of the core id
            if (m_readyQueue.empty()) {
                continue;
            }
            if (!core.isBusy) {                                     // If the core is idle
                Process* p_to_schedule = m_readyQueue.front();      // Get the front process (FCFS)
                m_readyQueue.pop();                                 // Remove it from the queue

                core.currentProcess = p_to_schedule;                // Assign the process to the core
                core.isBusy = true;                                 // Mark it as busy
                core.currentQuantumTicks = 0U;                      // Reset quantum counter for this newly assigned process
                p_to_schedule->setState(ProcessState::RUNNING);     // Set process as running so it can execute

                /*
                std::cout << "[Scheduler] Assigned Process " << p_to_schedule->getPname() << " (PID " << p_to_schedule->getPid() << ") to Core " << core.id << ".\n";
                */
            }
        }
    }
}

void Kernel::updateWaitingProcesses() {
    // Use a temporary list for processes that become ready
    std::vector<Process*> processesToMoveToReady;
    // Iterate through m_waitingQueue and identify those that are done sleeping
    for (Process* p : m_waitingQueue) { // Iterate m_waitingQueue directly
        p->decrementSleepTicks();
        if (p->getSleepTicksRemaining() == 0) {
            p->setState(ProcessState::READY);
            processesToMoveToReady.push_back(p); // Collect processes to move
        }
    }

    // Move processes from temporary list to m_readyQueue
    for (Process* p : processesToMoveToReady) {
        m_readyQueue.push(p);
    }

    // Remove processes that just became READY from m_waitingQueue
    m_waitingQueue.erase(std::remove_if(m_waitingQueue.begin(), m_waitingQueue.end(),
                                            [](Process* p) {
                                                return p->getState() == ProcessState::READY;
                                            }), m_waitingQueue.end());
}

bool Kernel::isBusy() {
    if (!m_readyQueue.empty() || !m_waitingQueue.empty()) { // Check if either ready queue or waiting processes has items
        return true;
    }
    for (auto& core : m_cpuCores) {
        if (core.isBusy) {
            return true;
        }
    }
    return false;
}

void Kernel::displayProcess(Process* process) const {
    std::cout << "Process Name: " << process->getPname() << "\n";
    std::cout << "ID: " << process->getPid() << "\n";
    std::cout << "Logs:\n";
    const auto& logBuffer = process->getLogBuffer();
    if (logBuffer.empty()) {
        std::cout << "Process log is empty.\n";
    } else {
        for (const std::string& logEntry : logBuffer) { // Print each entry in the log
            std::cout << logEntry << "\n";
        }
    }
    std::cout << "--- End of process log ---\n";
    std::cout << "Current instruction line: " << process->getCurrentInstructionLine() << "\n";
    std::cout << "Lines of code: " << process->getTotalInstructionLines() << "\n";
    std::cout << "Memory Required: " << process->getMemoryRequired() << "\n";
}
