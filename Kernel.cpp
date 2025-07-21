#include "Kernel.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <random>
#include <limits>

Kernel::Kernel()
    : m_nextPid(0),
      m_cpuTicks(0ULL),
      m_running(false),
      m_shutdownRequested(false),
      m_numCpus(4U),
      m_schedulerType(SchedulerType::ROUND_ROBIN),
      m_quantumCycles(5U),
      m_batchProcessFreq(1U),
      m_minInstructions(1000U),
      m_maxInstructions(2000U),
      m_delaysPerExec(0U),
      m_maxOverallMem(64U),
      m_memPerFrame(16U),
      m_memPerProc(64U) {}

// Core Lifecycle Methods
void Kernel::initialize(const SystemConfig& config) {
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
    m_memPerProc = config.memPerProc;

    // --- Initialize CPU Cores ---
    m_cpuCores.clear(); // Clear any existing cores
    m_cpuCores.resize(m_numCpus); // Create the specified number of cores
    for (uint32_t i = 0; i < m_numCpus; ++i) {
        m_cpuCores[i].id = i;
        m_cpuCores[i].currentProcess = nullptr; // No process initially assigned
        m_cpuCores[i].isBusy = false;
    }

    std::cout << "Kernel: Kernel initialized with " << m_numCpus << " CPU cores.\n";
    /*
    for (const auto& core: m_cpuCores) {
        std::cout << "Core " << core.id << "\n";
    }
    */
}

void Kernel::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_kernelMutex);
        std::cout << "Kernel: Shutting down all processes and background services.\n";
        m_running.store(false);             // Stop new process generation
        m_shutdownRequested.store(true);    // Signal that shutdown has been requested
        m_processes.clear();
        std::cout << "Kernel: System shutdown complete.\n";
    }
    m_cv.notify_one(); // Wake up run() thread if it's waiting
}

void Kernel::run() {
    while (true) {
        std::unique_lock<std::mutex> lock(m_kernelMutex);

        // If nothing to do (no new processes to generate, no existing processes to schedule)
        // AND not in the final shutdown phase, then wait.
        if (!m_running && m_processes.empty()) {
            m_cv.wait(lock); // Atomically releases the lock and waits. Reacquires lock on wake-up.
        }

        // IMMEDIATE TERMINATION: If m_shutdownRequested is true, break out of the loop.
        if (m_shutdownRequested) {
            break;
        }

        // Process Generation Logic: Only generate if m_running is true
        if (m_running) {
            if (m_cpuTicks % m_batchProcessFreq == 0) {
                int newPid = m_nextPid++; // Get next available PID and increment
                std::string newPname = "process" + std::to_string(newPid);
                generateDummyProcess(newPname, newPid);
            }
            if (m_cpuTicks % m_delaysPerExec == 0) {
                //scheduleProcesses();
            }
            m_cpuTicks++;
        }

        lock.unlock(); // Release the lock before sleeping

        // Small sleep to prevent busy-waiting.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Dummy Process Generator
void Kernel::generateDummyProcess(const std::string& newPname, int newPid) {
    // No need for mutex because it's only called within run()
    // Use a random number generator
    // static ensures the generator is initialized only once per program run
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // 1. Determine random number of instructions within the specified range
    // Ensure m_minInstructions and m_maxInstructions are initialized (e.g., in Kernel constructor or initialize method)
    if (m_minInstructions == 0 || m_maxInstructions == 0 || m_minInstructions > m_maxInstructions) {
        std::cerr << "[Kernel] Warning: Invalid instruction range (" << m_minInstructions
                  << "-" << m_maxInstructions << "). Using default range [10, 50].\n";
        m_minInstructions = 10; // Fallback to a sensible default
        m_maxInstructions = 50;
    }
    std::uniform_int_distribution<> distrib_num_instructions(m_minInstructions, m_maxInstructions);
    size_t numInstructions = distrib_num_instructions(gen);

    std::vector<std::unique_ptr<IProcessInstruction>> instructions;
    instructions.reserve(numInstructions);

    // A list of possible variable names to use in dummy processes
    const std::vector<std::string> varNames = {"a", "b", "c", "x", "y", "counter", "temp"};
    // A distribution for random values (0-65,535 for uint16_t)
    std::uniform_int_distribution<uint16_t> distrib_value(std::numeric_limits<uint16_t>::min(), std::numeric_limits<uint16_t>::max());
    // A distribution for sleep ticks (e.g., 1 to 255 ticks for uint8_t)
    std::uniform_int_distribution<uint8_t> distrib_sleep_ticks(1, 255);
    // A distribution for loop repeats (e.g., 1 to 3 repeats)
    std::uniform_int_distribution<int> distrib_loop_repeats(1, 3);


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
                            loopBody.push_back(std::make_unique<PrintInstruction>("Looping... +counter"));
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

    // Create the process and add it to the kernel's process list
    m_processes.push_back(std::make_unique<Process>(newPid, newPname, std::move(instructions)));

    /*
    std::cout << "[Kernel] Generated dummy process with PID: " << newPid
              << ". Instructions: " << numInstructions
              << ". Total processes: " << m_processes.size() << "\n";
    */
}

// Process Generation Control
void Kernel::startProcessGeneration() {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    if (!m_running) {
        m_running.store(true);
        std::cout << "Kernel: Process generation activated.\n";
        m_cv.notify_one(); // Wake up run() thread if it's waiting
    } else {
        std::cout << "Kernel: Process generation is already active.\n";
    }
}

void Kernel::stopProcessGeneration() {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    if (m_running) {
        m_running.store(false);
        std::cout << "Kernel: Process generation deactivated.\n";
    } else {
        std::cout << "Kernel: Process generation is already inactive.\n";
    }
}

// Screen Commands
void Kernel::listStatus() {
    std::lock_guard<std::mutex> lock(m_kernelMutex);
    if (m_processes.size() == 0) {
        std::cout << "No processes found." << "\n";
        return;
    }
    std::cout << "Processes:\n";
    for(const auto& process : m_processes) {
        std::cout << process->getPname() << ": (" << process->getCreationTime() << ") " << process->getCurrentInstructionLine() << "/" <<process->getTotalInstructionLines() << "\n";
    }
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

/* Old, not working scheduleProcesses
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
            process_ptr->executeNextInstruction();   // Execute one instruction

            // If the process is not yet terminated, put it back to READY or WAITING
            if (process_ptr->getState() == ProcessState::RUNNING) { // Still running after instruction
                process_ptr->setState(ProcessState::READY); // For round-robin, put back to READY
            }
            // If it transitioned to TERMINATED or WAITING, leave it in that state.
            break; // Only execute one process per tick for simplicity
        }
    }
}
*/

/* Not used
Process* Kernel::findProcessByPid(int pid) const {
    // This method is called from within a critical section (e.g., getProcessStatus, scheduleProcesses), which already holds m_kernelMutex
    for (const auto& p : m_processes) {
        if (p->getPid() == pid) {
            return p.get(); // Return raw pointer to the Process object
        }
    }
    return nullptr; // Process not found
}
*/
