#pragma once
#include <string>
#include <iostream>

class Screen {
    std::string processName;
    std::string timestamp;
    int currentLine;
    int totalLines;
public:
    Screen(const std::string& name, const std::string& time, int total = 5) //placeholder 5 lines
        : processName(name), timestamp(time), currentLine(1), totalLines(total) {}

    void viewSession() const {
        system("cls");
        std::cout << "Process Name: " << processName << "\n";
        std::cout << "Instruction Line: " << currentLine << " / " << totalLines << "\n"; //hardcoded for now
        std::cout << "Timestamp: " << timestamp << "\n";
        std::cout << "Type 'exit' to return to main menu.\n";
    }

    void viewSummary() const {
        std::cout << "Process Name: " << processName << " | Created: " << timestamp << std::endl;
    }

    std::string getprocessName() const { return processName; }
};
