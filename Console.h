#ifndef CONSOLE_H
#define CONSOLE_H

#include <iostream>
#include <string>
#include <cstdlib>
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
            system("cls");
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