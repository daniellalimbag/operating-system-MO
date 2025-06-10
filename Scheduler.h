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
private:
    static const int NUM_CORES = 4;
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
                    handleInstruction(process, instr, core);
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

    void handleInstruction(Process* process, const std::string& instr, int core) {
        // Handle PRINT command: PRINT("message")
        if (instr.find("PRINT(") == 0) {
            size_t start = instr.find('"');
            size_t end = instr.rfind('"');
            std::string msg = (start != std::string::npos && end != std::string::npos && end > start)
                ? instr.substr(start + 1, end - start - 1) : instr;
            logPrint(process, msg, core);
        }
        // Other instructions can be handled here
    }

    void logPrint(Process* process, const std::string& msg, int core) {
        std::ofstream ofs(getLogFileName(process), std::ios::app);
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm;
        localtime_s(&local_tm, &now_time_t);
        // Format: (MM/DD/YYYY, HH:MM:SS AM/PM) Core: <core> "message"
        ofs << "(" << std::put_time(&local_tm, "%m/%d/%Y, %I:%M:%S %p") << ") Core: " << core << " " << msg << std::endl;
    }

    std::string getLogFileName(Process* process) {
        return process->getProcessName() + ".txt";
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