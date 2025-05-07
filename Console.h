#ifndef CONSOLE_H
#define CONSOLE_H

#include <iostream>
#include <string>
#include "Header.h"

class OpesyConsole {
public:
    OpesyConsole() {}

    void displayHeader() {
        std::cout << CSOPESY_HEADER << std::endl;
        std::cout << "Hello! Welcome to CSOPESY commandline!" << std::endl;
        std::cout << "Type 'exit' to quit, 'clear' to refresh the screen." << std::endl;
        std::cout << std::endl;
    }

    void processCommand(std::string command) {
        if (command == "initialize") {
            std::cout << command << " command recognized. Doing something." << std::endl;
        }
        else if (command == "screen") {
            std::cout << command << " command recognized. Doing something." << std::endl;
        }
        else if (command == "scheduler-test") {
            std::cout << command << " command recognized. Doing something." << std::endl;
        }
        else if (command == "scheduler-stop") {
            std::cout << command << " command recognized. Doing something." << std::endl;
        }
        else if (command == "report-util") {
            std::cout << command << " command recognized. Doing something." << std::endl;
        }
        else if (command == "clear") {
            displayHeader();
        }
        else if (command == "exit") {
            std::cout << "Exiting CSOPESY CLI..." << std::endl;
            exit(0);
        }
        else {
            std::cout << command << " command unrecognized. Enter a valid command." << std::endl;
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