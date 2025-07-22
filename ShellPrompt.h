#pragma once

#include <string>

class Kernel; // Forward declaration of Kernel class.
class Process;

/**
 * @class ShellPrompt
 * @brief Manages the command-line interface, user input, and interaction with the Kernel.
 */
class ShellPrompt {
public:
    explicit ShellPrompt(Kernel& kernel);
    void start();

private:
    Kernel& kernel; // Reference to the OS kernel for accessing its services.

    void processCommand(const std::string& command);
    void screenMenu(Process* process);
    void showHelp() const;
};
