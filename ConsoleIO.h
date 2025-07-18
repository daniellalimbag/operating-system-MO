#pragma once

#include <string>

namespace ConsoleIO {
    void PrintHeader();
    std::string GetInput(const std::string& prompt);
    void PrintLine(const std::string& message);
    void DrawHorizontalLine();
    void ClearScreen();
}
