#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <memory>

extern std::atomic<uint64_t> cpuTickCount; // Global CPU tick counter

#include <fstream>
#include <chrono>
#include "ProcessManager.h"
#include "Config.h"
#include "FirstFitMemoryAllocator.h"

enum class SchedulingAlgorithm {
    FCFS,
    ROUND_ROBIN
};

class Scheduler {
private:
    ProcessManager& processManager;
    std::queue<int> readyQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    std::thread schedulerThread;
    std::vector<std::thread> workerThreads;
    std::vector<std::unique_ptr<std::atomic<bool>>> coreBusy;
    std::vector<std::unique_ptr<std::atomic<int>>> coreProcess;
    std::vector<std::unique_ptr<std::atomic<int>>> coreQuantumRemaining;
    std::atomic<bool> running;
    SchedulingAlgorithm algorithm;
    SystemConfig config;
    int numCores;
    std::vector<std::pair<int, int>> waitingQueue;

    int quantumCycles;
    int delayPerExec;

    void scheduleFCFS();
    void scheduleRR();
    void assignProcessToCore(int pid, int core);
    int calculateCoreUtilization();
    void schedulerLoop();
    void workerLoop(int core);

public:
    Scheduler(ProcessManager& pm);
    ~Scheduler();
    void updateConfig(const SystemConfig& newConfig);

    void initializeCores();
    void start();
    void stop();
    void addProcess(int pid);
    bool isCoreBusy(int core) const;
    int getNumCores() const;
    SchedulingAlgorithm getAlgorithm() const;
    bool isRunning() const;
    int getCoreProcess(int core) const;
    int getCoreQuantumRemaining(int core) const;
    size_t getReadyQueueSize();
    uint64_t getCurrentTick() const { return cpuTickCount.load(); }
    void checkWaitingQueue();
    void addToWaitingQueue(int pid, int sleepTicks);
};