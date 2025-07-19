#include "Kernel.h"
#include <iostream>
#include <limits>

Kernel::Kernel() {
    // In a real OS, this constructor would perform critical low-level tasks:
    // - Initialize hardware (CPU, memory controller, timers, I/O devices).
    // - Set up interrupt handlers.
    // - Initialize memory management units.
    // - Load initial system processes/drivers.
}

void Kernel::print(const std::string& message) const {
    std::cout << message;
}

std::string Kernel::readLine(const std::string& prompt) const {
    std::string inputLine;
    this->print(prompt);
    std::getline(std::cin, inputLine);

    return inputLine;
}

void Kernel::clearScreen() const {
    const std::string ANSI_CLEAR_SCREEN = "\033[2J\033[H";
    std::cout << ANSI_CLEAR_SCREEN;
}
