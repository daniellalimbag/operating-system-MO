#pragma once

#include <string>
#include <functional>
#include <vector>
#include <map>

class Kernel;
class Process;
struct ParsedCommand;

using CommandHandler = std::function<bool(const std::vector<std::string>&)>;

/**
 * @class ShellPrompt
 * @brief Manages the command-line interface, user input, and interaction with the Kernel.
 */
class ShellPrompt {
public:
    explicit ShellPrompt(Kernel& kernel);
    void run();

private:
    Kernel& kernel;
    std::map<std::string, CommandHandler> commandHandlers;

    void showHeader(const std::string& instructions) const;
    bool runInitialBootPrompt(const std::string& prompt);
    void initializeKernel();
    void runMainShellLoop(const std::string& prompt);
    ParsedCommand parseCommand(const std::string& command);
    bool executeCommand(const ParsedCommand& parsed);
    void showHelp() const;
    void setupCommands();
    void handleScreenReattach(const std::vector<std::string>& args);
    void handleScreenStart(const std::vector<std::string>& args);
    void handleScreenMenu(Process* process);
};
