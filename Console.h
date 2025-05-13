#ifndef CONSOLE_H
#define CONSOLE_H

#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include "Header.h"

class OpesyConsole {
public:
    OpesyConsole() {}

    void displayHeader() {
        std::cout << CSOPESY_HEADER << std::endl;
        std::cout << "\033[32mHello! Welcome to CSOPESY commandline!\033[0m" << std::endl;
        std::cout << "\033[93mType 'exit' to quit, 'clear' to refresh the screen.\033[0m" << std::endl;
        std::cout << std::endl;
    }

    bool validateCommand(const std::string& command) {
        static const std::vector<std::string> validCommands = {
            "initialize", "screen", "scheduler-test", "scheduler-stop", "report-util"
        };
        
        return std::find(validCommands.begin(), validCommands.end(), command) != validCommands.end();
    }

    void executeCommand(const std::string& command) {
        std::cout << command << " command recognized. Doing something." << std::endl;
    }

    void processCommand(const std::string& command) {
        if (command == "clear") {
            system("cls");
            displayHeader();
        } else if (command == "exit") {
            std::cout << "Exiting CSOPESY CLI..." << std::endl;
            exit(0);
        } else {
            if (validateCommand(command)) {
                executeCommand(command);
            } else {
                std::cout << command << " command unrecognized. Enter a valid command." << std::endl;
            }
        }
        std::cout << std::endl;
    }

    void run() {
        std::string command;
        displayHeader();
        
        while (true) {
            std::cout << "Enter a command: ";
            std::getline(std::cin, command);
            processCommand(command);
        }
    }
};

#endif