// Fixed Scheduler.cpp
#include "Scheduler.h"
#include <atomic>
#include <thread>
#include <chrono>
#include "FirstFitMemoryAllocator.h"

// Global CPU tick counter
std::atomic<uint64_t> cpuTickCount{0};

// Waiting queue
void Scheduler::addToWaitingQueue(int pid, int sleepTicks) {
    std::lock_guard<std::mutex> lock(queueMutex);
    waitingQueue.push_back({pid, sleepTicks});
}

void Scheduler::checkWaitingQueue() {
    std::lock_guard<std::mutex> lock(queueMutex);
    for (auto it = waitingQueue.begin(); it != waitingQueue.end(); ) {
        int pid = it->first;
        int& ticks = it->second;
        Process* process = processManager.getProcess(pid);
        if (process && process->getSleepTicks() > 0) {
            process->setSleepTicks(process->getSleepTicks() - 1);
            ticks = process->getSleepTicks();
        }
        if (!process || process->getSleepTicks() <= 0) {
            readyQueue.push(pid);
            it = waitingQueue.erase(it);
        } else {
            ++it;
        }
    }
}

Scheduler::Scheduler(ProcessManager& pm)
    : processManager(pm), running(false), algorithm(SchedulingAlgorithm::FCFS),
      numCores(4), quantumCycles(1), delayPerExec(0) {
    initializeCores();
}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    if (running) return;
    
    quantumCycleCounter.store(0);
    lastSnapshotTick = 0;
    currentQuantumTick = 0;
    
    running = true;
    
    // Start scheduler loop (for process scheduling)
    schedulerThread = std::thread(&Scheduler::schedulerLoop, this);
    
    // Start worker threads (for instruction execution and CPU ticks)
    workerThreads.clear();
    for (int i = 0; i < numCores; ++i) {
        workerThreads.emplace_back(&Scheduler::workerLoop, this, i);
    }
}

void Scheduler::stop() {
    if (!running) return;
    running = false;
    cv.notify_all();

    if (schedulerThread.joinable()) schedulerThread.join();

    for (auto& t : workerThreads) {
        if (t.joinable()) t.join();
    }
    workerThreads.clear();

    for (int i = 0; i < numCores; ++i) {
        if (coreBusy[i]->load()) {
            int pid = coreProcess[i]->load();
            if (pid != -1) {
                processManager.assignProcessToCore(pid, -1);
            }
            coreBusy[i]->store(false);
            coreProcess[i]->store(-1);
            coreQuantumRemaining[i]->store(0);
        }
    }
}

void Scheduler::updateConfig(const SystemConfig& newConfig) {
    bool wasRunning = running.load();
    if (wasRunning) stop();
    
    config = newConfig;
    numCores = newConfig.numCPU;
    quantumCycles = std::max(uint32_t(1), newConfig.quantumCycles);
    delayPerExec = std::max(uint32_t(0), newConfig.delaysPerExec);
    
    if (newConfig.scheduler == "rr") {
        algorithm = SchedulingAlgorithm::ROUND_ROBIN;
    } else {
        algorithm = SchedulingAlgorithm::FCFS;
    }
    
    initializeCores();
    if (wasRunning) start();
}

void Scheduler::initializeCores() {
    coreBusy.clear();
    coreProcess.clear();
    coreQuantumRemaining.clear();
    
    for (int i = 0; i < numCores; ++i) {
        coreBusy.push_back(std::make_unique<std::atomic<bool>>(false));
        coreProcess.push_back(std::make_unique<std::atomic<int>>(-1));
        coreQuantumRemaining.push_back(std::make_unique<std::atomic<int>>(0));
    }
}

void Scheduler::addProcess(int pid) {
    std::lock_guard<std::mutex> lock(queueMutex);
    readyQueue.push(pid);
    cv.notify_one();
}

bool Scheduler::isCoreBusy(int core) const {
    if (core < 0 || core >= numCores) return false;
    return coreBusy[core]->load();
}

int Scheduler::getNumCores() const {
    return numCores;
}

SchedulingAlgorithm Scheduler::getAlgorithm() const {
    return algorithm;
}

bool Scheduler::isRunning() const {
    return running;
}

int Scheduler::getCoreProcess(int core) const {
    if (core < 0 || core >= numCores) return -1;
    return coreProcess[core]->load();
}

int Scheduler::getCoreQuantumRemaining(int core) const {
    if (core < 0 || core >= numCores) return 0;
    return coreQuantumRemaining[core]->load();
}

size_t Scheduler::getReadyQueueSize() {
    std::lock_guard<std::mutex> lock(queueMutex);
    return readyQueue.size();
}

void Scheduler::scheduleFCFS() {
    std::lock_guard<std::mutex> lock(queueMutex);
    for (int core = 0; core < numCores; ++core) {
        if (!coreBusy[core]->load() && !readyQueue.empty()) {
            int pid = readyQueue.front();
            readyQueue.pop();
            
            Process* process = processManager.getProcess(pid);
            if (process && !process->isComplete()) {
                // Check memory allocation
                extern FirstFitMemoryAllocator* globalMemoryAllocator;
                if (globalMemoryAllocator && !globalMemoryAllocator->isAllocated(pid)) {
                    if (!globalMemoryAllocator->allocate(pid)) {
                        // Memory not available, put back in queue
                        readyQueue.push(pid);
                        continue;
                    }
                }
                assignProcessToCore(pid, core);
            }
        }
    }
}

void Scheduler::scheduleRR() {
    std::lock_guard<std::mutex> lock(queueMutex);
    
    // Check for processes that need to be preempted or completed
    for (int core = 0; core < numCores; ++core) {
        if (coreBusy[core]->load()) {
            int currentPid = coreProcess[core]->load();
            Process* currentProcess = processManager.getProcess(currentPid);
            
            bool shouldPreempt = false;
            
            if (!currentProcess || currentProcess->isComplete()) {
                shouldPreempt = true;
                if (currentProcess && currentProcess->isComplete()) {
                    // Process completed, release memory
                    extern FirstFitMemoryAllocator* globalMemoryAllocator;
                    if (globalMemoryAllocator && globalMemoryAllocator->isAllocated(currentPid)) {
                        globalMemoryAllocator->release(currentPid);
                    }
                }
                if (currentProcess) {
                    processManager.assignProcessToCore(currentPid, -1);
                }
            }
            else if (coreQuantumRemaining[core]->load() <= 0) {
                shouldPreempt = true;
                processManager.assignProcessToCore(currentPid, -1);
                readyQueue.push(currentPid);
            }
            
            if (shouldPreempt) {
                coreBusy[core]->store(false);
                coreProcess[core]->store(-1);
                coreQuantumRemaining[core]->store(0);
            }
        }
    }
    
    // Assign new processes to available cores
    for (int core = 0; core < numCores; ++core) {
        if (!coreBusy[core]->load() && !readyQueue.empty()) {
            int pid = readyQueue.front();
            readyQueue.pop();
            
            Process* process = processManager.getProcess(pid);
            if (process && !process->isComplete()) {
                // Check memory allocation
                extern FirstFitMemoryAllocator* globalMemoryAllocator;
                if (globalMemoryAllocator && !globalMemoryAllocator->isAllocated(pid)) {
                    if (!globalMemoryAllocator->allocate(pid)) {
                        // Memory not available, put back in queue
                        readyQueue.push(pid);
                        continue;
                    }
                }
                assignProcessToCore(pid, core);
                coreQuantumRemaining[core]->store(quantumCycles);
            }
        }
    }
}

void Scheduler::assignProcessToCore(int pid, int core) {
    if (core < 0 || core >= numCores || coreBusy[core]->load()) {
        return;
    }
    coreProcess[core]->store(pid);
    coreBusy[core]->store(true);
    processManager.assignProcessToCore(pid, core);
    
    int utilization = calculateCoreUtilization();
    processManager.updateProcessUtilization(pid, utilization);
}

int Scheduler::calculateCoreUtilization() {
    int busyCores = 0;
    for (int i = 0; i < numCores; ++i) {
        if (coreBusy[i]->load()) busyCores++;
    }
    return (numCores == 0) ? 0 : (busyCores * 100 / numCores);
}

void Scheduler::schedulerLoop() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (!running) break;
        
        checkWaitingQueue();
        
        switch (algorithm) {
            case SchedulingAlgorithm::FCFS:
                scheduleFCFS();
                break;
            case SchedulingAlgorithm::ROUND_ROBIN:
                scheduleRR();
                break;
        }
        
        // Check if we need to take a snapshot for RR
        if (algorithm == SchedulingAlgorithm::ROUND_ROBIN) {
            checkAndTakeSnapshot();
        }
    }
}

void Scheduler::checkAndTakeSnapshot() {
    currentQuantumTick++;
    
    // Take snapshot every quantum cycles
    if (currentQuantumTick >= quantumCycles) {
        currentQuantumTick = 0;
        if (memorySnapshotCallback) {
            int snapshotNum = quantumCycleCounter.fetch_add(1) + 1;
            memorySnapshotCallback(snapshotNum);
        }
    }
}

void Scheduler::workerLoop(int core) {
    while (running) {
        cpuTickCount++;

        if (coreBusy[core]->load()) {
            int pid = coreProcess[core]->load();
            Process* process = processManager.getProcess(pid);

            if (process) {
                if (!process->isComplete()) {
                    // Handle sleep
                    if (process->getSleepTicks() > 0) {
                        {
                            std::lock_guard<std::mutex> lock(queueMutex);
                            waitingQueue.push_back({pid, process->getSleepTicks()});
                        }
                        processManager.assignProcessToCore(pid, -1);
                        coreBusy[core]->store(false);
                        coreProcess[core]->store(-1);
                        coreQuantumRemaining[core]->store(0);
                        continue;
                    }

                    // Execute instruction
                    std::string result = process->executeNextInstruction();

                    // Decrement quantum for RR
                    if (algorithm == SchedulingAlgorithm::ROUND_ROBIN) {
                        int remaining = coreQuantumRemaining[core]->load();
                        if (remaining > 0) {
                            coreQuantumRemaining[core]->store(remaining - 1);
                        }
                    }

                    if (delayPerExec > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));
                    }
                } else {
                    // Process completed
                    processManager.assignProcessToCore(pid, -1);
                    extern FirstFitMemoryAllocator* globalMemoryAllocator;
                    if (globalMemoryAllocator && globalMemoryAllocator->isAllocated(pid)) {
                        globalMemoryAllocator->release(pid);
                    }
                    coreBusy[core]->store(false);
                    coreProcess[core]->store(-1);
                    coreQuantumRemaining[core]->store(0);
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}