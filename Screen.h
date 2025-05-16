#pragma once
#include <string>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

class Screen {
    std::string processName;
    std::string timestamp;
    int currentLine;
    int totalLines;

public:
    Screen() = delete;
    Screen(const std::string& name, int total = 5)
        : processName(name), currentLine(1), totalLines(total) {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm;
        localtime_s(&local_tm, &now_time_t);
        std::ostringstream oss;
        oss << std::put_time(&local_tm, "%m/%d/%Y, %I:%M:%S %p");
        timestamp = oss.str();
    }

    void viewSession() const {
        system("cls");
        std::cout << "Process Name: " << processName << "\n";
        std::cout << "Instruction Line: " << currentLine << " / " << totalLines << "\n";
        std::cout << "Timestamp: " << timestamp << "\n";
    }

    void viewSummary() const {
        std::cout << "Process Name: " << processName << " | Created: " << timestamp << std::endl;
    }

    std::string getProcessName() const { return processName; }
    std::string getTimestamp() const { return timestamp; }
    int getCurrentLine() const { return currentLine; }
    int getTotalLines() const { return totalLines; }
};