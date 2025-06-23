#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <fstream>
#include "ProcessManager.h"

enum class SchedulingAlgorithm {
    FCFS,
    //to be added
};

class Scheduler {
public:
    static const int NUM_CORES = 4;
    bool isCoreBusy(int core) const { return coreBusy[core]; }
private:
    ProcessManager& processManager;
    std::queue<int> readyQueue; //stores the PIDs of the processes
    std::mutex queueMutex;
    std::condition_variable cv;
    std::thread schedulerThread;
    std::vector<std::thread> workerThreads;
    std::vector<std::atomic<bool>> coreBusy;
    std::vector<std::atomic<int>> coreProcess;
    std::atomic<bool> running;
    SchedulingAlgorithm algorithm;

    void scheduleFCFS() {
        for (int core = 0; core < NUM_CORES; ++core) {
            if (!coreBusy[core] && !readyQueue.empty()) {
                int pid = readyQueue.front();
                readyQueue.pop();
                coreBusy[core] = true;
                coreProcess[core] = pid;
                processManager.assignProcessToCore(pid, core);
            }
        }
    }

    void schedulerLoop() {
        while (running) {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [&] { return !readyQueue.empty() || !running; });
            if (!running) break;
            switch (algorithm) {
                case SchedulingAlgorithm::FCFS:
                    scheduleFCFS();
                    break;
                // case SchedulingAlgorithm::ROUND_ROBIN:
                //     scheduleRoundRobin();
                //     break;
            }
        }
    }

    void workerLoop(int core) {
        while (running) {
            if (coreBusy[core]) {
                int pid = coreProcess[core];
                Process* process = processManager.getProcess(pid);
                if (process && !process->isComplete()) {
                    std::string instr = process->executeNextInstruction();
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
                if (process && process->isComplete()) {
                    coreBusy[core] = false;
                    coreProcess[core] = -1;
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
    }

public:
    Scheduler(ProcessManager& pm, SchedulingAlgorithm algo = SchedulingAlgorithm::FCFS)
        : processManager(pm), running(false), coreBusy(NUM_CORES), coreProcess(NUM_CORES), algorithm(algo) {
        for (int i = 0; i < NUM_CORES; ++i) {
            coreBusy[i] = false;
            coreProcess[i] = -1;
        }
    }

    ~Scheduler() { stop(); }

    void start() {
        running = true;
        schedulerThread = std::thread(&Scheduler::schedulerLoop, this);
        for (int i = 0; i < NUM_CORES; ++i) {
            workerThreads.emplace_back(&Scheduler::workerLoop, this, i);
        }
    }

    void stop() {
        running = false;
        cv.notify_all();
        if (schedulerThread.joinable()) schedulerThread.join();
        for (auto& t : workerThreads) {
            if (t.joinable()) t.join();
        }
    }

    void addProcess(int pid) {
        std::lock_guard<std::mutex> lock(queueMutex);
        readyQueue.push(pid);
        cv.notify_one();
    }
}; 