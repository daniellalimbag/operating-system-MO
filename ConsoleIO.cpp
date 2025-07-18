#include <string>
#include <iostream>
#include <algorithm>
#include <cctype>
#include "ConsoleIO.h"

static const char* CSOPESY_ASCII_ART = R"(
  ___  ____   __  ____  ____  ____  _  _
 / __)/ ___) /  \(  _ \(  __)/ ___)( \/ )
( (__ \___ \(  O )) __/ ) _) \___ \ )  /
 \___)(____/ \__/(__)  (____)(____/(__/
)";

namespace ConsoleIO {
    namespace { // Anonymous namespace to give internal linkage to its contents
        void trim_whitespace_helper(std::string& s) {
            // Trim leading whitespace
            s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), [](unsigned char ch) {
                return std::isspace(ch);
            }));
            // Trim trailing whitespace
            s.erase(std::find_if_not(s.rbegin(), s.rend(), [](unsigned char ch) {
                return std::isspace(ch);
            }).base(), s.end());
        }

        const char HORIZONTAL_LINE_CHAR = '-';
        const int HORIZONTAL_LINE_LENGTH = 40;
    }
    void PrintHeader() {
        std::cout << CSOPESY_ASCII_ART << "\n";
    }

    std::string GetInput(const std::string& prompt) {
        std::string input;
        std::cout << prompt;
        std::getline(std::cin, input);

        trim_whitespace_helper(input);

        return input;
    }

    void PrintLine(const std::string& message) {
        std::cout << message << "\n";
    }

    void DrawHorizontalLine() {
        for (int i = 0; i < HORIZONTAL_LINE_LENGTH; ++i) {
            std::cout << HORIZONTAL_LINE_CHAR;
        }
        std::cout << "\n";
    }

    void ClearScreen() {
        #ifdef _WIN32
            system("cls"); // For Windows
        #else // macOS, Linux, and other Unix-like systems
            system("clear");
        #endif
    }
}
