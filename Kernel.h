#pragma once

#include <string>

/**
 * @class Kernel
 * @brief Core OS kernel responsible for OS management, and exposes public API for system interaction
 */
// Kernel.h
class Kernel {
public:
    Kernel();

    // I/O APIs
    void print(const std::string& message) const;
    std::string readLine(const std::string& prompt) const;
    void clearScreen() const;
};
