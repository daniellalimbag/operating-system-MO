#ifndef CONSOLE_H
#define CONSOLE_H

#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <map>
#include <ctime>
#include "Header.h"
#include "Screen.h"

class OpesyConsole {
private:
    // Map to store active screen sessions
    std::map<std::string, Screen> screens;

public:
    OpesyConsole() {}

    void displayHeader() {
        std::cout << CSOPESY_HEADER << std::endl;
        std::cout << "\033[32mHello! Welcome to CSOPESY commandline!\033[0m" << std::endl;
        std::cout << "\033[93mType 'exit' to quit, 'clear' to refresh the screen.\033[0m" << std::endl;
        std::cout << std::endl;
    }

    // Validate if a command is recognized
    bool validateCommand(const std::string& command) {
        static const std::vector<std::string> validCommands = {
            "initialize", "screen -ls", "scheduler-test", "scheduler-stop", "report-util", "exit", "clear"
        };

        if (command.rfind("screen -s ", 0) == 0 || command.rfind("screen -r ", 0) == 0) {
            return true;
        }

        return std::find(validCommands.begin(), validCommands.end(), command) != validCommands.end();
    }

    // Execute a recognized command
    void executeCommand(const std::string& command) {
        if (command.rfind("screen -s ", 0) == 0) {
            // Create a new screen
            std::string sessionName = command.substr(10);
            if (screens.count(sessionName) == 0) {
                screens.emplace(sessionName, Screen(sessionName));
                sessionLoop(sessionName);
            } else {
                std::cout << "Screen session '" << sessionName << "' already exists." << std::endl;
            }
        } else if (command.rfind("screen -r ", 0) == 0) {
            // Resume an existing screen
            std::string sessionName = command.substr(10);
            if (screens.find(sessionName) != screens.end()) {
                sessionLoop(sessionName);
            } else {
                std::cout << "Screen session '" << sessionName << "' does not exist." << std::endl;
            }
        } else if (command == "screen -ls") {
            // List all active screen sessions
            if (screens.empty()) {
                std::cout << "No active screen sessions." << std::endl;
            } else {
                std::cout << "Active screen sessions:" << std::endl;
                for (const auto& [name, screen] : screens) {
                    screen.viewSummary();
                }
            }
        } else {
            std::cout << command << " command recognized. Doing something." << std::endl;
        }
    }

    void sessionLoop(const std::string& sessionName) {
        auto it = screens.find(sessionName);
        if (it == screens.end()) return;
        Screen& session = it->second;
        std::string input;
        while (true) {
            session.viewSession();
            std::cout << std::endl;
            std::cout << "root:/> ";
            std::getline(std::cin, input);
            if (input == "exit") {
                system("cls");
                displayHeader();
                break;
            } else {
                std::cout << "Unknown command. Type 'exit' to return to main menu.\n";
            }
        }
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

    // Main loop
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