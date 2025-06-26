#include "Scheduler.h"
#include <atomic>
#include <thread>
#include <chrono>

// Global CPU tick counter
std::atomic<uint64_t> cpuTickCount{0};

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
}

void Scheduler::updateConfig(const SystemConfig& newConfig) {
    bool wasRunning = running.load();
    if (wasRunning) stop();
    
    config = newConfig;
    numCores = newConfig.numCPU;
    quantumCycles = std::max(1, newConfig.quantumCycles);
    delayPerExec = std::max(0, newConfig.delaysPerExec);
    
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
            assignProcessToCore(pid, core);
        }
    }
}

void Scheduler::scheduleRR() { // Might need checking
    std::lock_guard<std::mutex> lock(queueMutex);
    
    for (int core = 0; core < numCores; ++core) {
        if (coreBusy[core]->load() && coreQuantumRemaining[core]->load() <= 0) {
            int currentPid = coreProcess[core]->load();
            Process* currentProcess = processManager.getProcess(currentPid);
            
            if (currentProcess && !currentProcess->isComplete()) {
                readyQueue.push(currentPid);
            }
            
            coreBusy[core]->store(false);
            coreProcess[core]->store(-1);
            coreQuantumRemaining[core]->store(0);
        }
    }
    
    for (int core = 0; core < numCores; ++core) {
        if (!coreBusy[core]->load() && !readyQueue.empty()) {
            int pid = readyQueue.front();
            readyQueue.pop();
            assignProcessToCore(pid, core);
            coreQuantumRemaining[core]->store(quantumCycles);
        }
    }
}

void Scheduler::assignProcessToCore(int pid, int core) {
    coreBusy[core]->store(true);
    coreProcess[core]->store(pid);
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
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait_for(lock, std::chrono::milliseconds(200), [&] { 
            return !readyQueue.empty() || !running; 
        });
        
        if (!running) break;
        lock.unlock();
        
        switch (algorithm) {
            case SchedulingAlgorithm::FCFS:
                scheduleFCFS();
                break;
            case SchedulingAlgorithm::ROUND_ROBIN:
                scheduleRR();
                break;
        }
    }
}

void Scheduler::workerLoop(int core) {
    auto lastTickTime = std::chrono::steady_clock::now();
    const auto tickInterval = std::chrono::milliseconds(100); // 10 ticks per second
    
    while (running) {
        auto currentTime = std::chrono::steady_clock::now();
        
        if (currentTime - lastTickTime >= tickInterval) {
            if (core == 0) {
                cpuTickCount++;
            }
            lastTickTime = currentTime;
            
            if (coreBusy[core]->load()) {
                int pid = coreProcess[core]->load();
                Process* process = processManager.getProcess(pid);
                if (process && !process->isComplete()) {
                    std::string result = process->executeNextInstruction();
                    
                    if (algorithm == SchedulingAlgorithm::ROUND_ROBIN) {
                        int remaining = coreQuantumRemaining[core]->load();
                        if (remaining > 0) {
                            coreQuantumRemaining[core]->store(remaining - 1);
                        }
                    }
                }
                
                if (process && process->isComplete()) {
                    coreBusy[core]->store(false);
                    coreProcess[core]->store(-1);
                    coreQuantumRemaining[core]->store(0);
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}