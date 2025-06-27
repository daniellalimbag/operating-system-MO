#include "MarqueeConsole.h"
#include <iostream>
#include <functional>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <cstdlib>
#endif

MarqueeConsole::MarqueeConsole() : x(0), y(HEADER_LINES), dx(1), dy(1) {
    marqueeRunning = false;
}

MarqueeConsole::~MarqueeConsole() {
    stop();
}

void MarqueeConsole::setCursorPosition(int x, int y) {
    #ifdef _WIN32
    COORD pos = { static_cast<SHORT>(x), static_cast<SHORT>(y) };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
    #else
    printf("\033[%d;%dH", y + 1, x + 1);
    #endif
}

void MarqueeConsole::getConsoleSize(int& width, int& height) {
    #ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    #else
    // For Linux/Mac, you'd need to implement terminal size detection
    width = 80;  // Default fallback
    height = 24;
    #endif
}

void MarqueeConsole::displayMarqueeHeader() {
    setCursorPosition(0, 0);
    std::cout << "*****************************************\n";
    std::cout << "* Displaying a marquee console!         *\n";
    std::cout << "*****************************************\n";
}

void MarqueeConsole::clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void MarqueeConsole::displayMainHeader() {
    // This should call the main console's displayHeader function
    // For now, we'll just clear and show a simple message
    clearScreen();
    std::cout << "Returned to main console.\n";
}

void MarqueeConsole::runMarqueeAnimation() {
    using clock = std::chrono::steady_clock;
    auto lastFrame = clock::now();
    const auto frameTime = std::chrono::milliseconds(FRAME_DELAY);

    while (marqueeRunning) {
        auto currentTime = clock::now();
        auto elapsedTime = currentTime - lastFrame;

        if (elapsedTime >= frameTime) {
            {
                std::lock_guard<std::mutex> lock(consoleMutex);
                int consoleWidth, consoleHeight;
                getConsoleSize(consoleWidth, consoleHeight);
                
                int inputLine = consoleHeight - RESERVED_HISTORY_LINES - 1;
                if (y >= inputLine) y = inputLine - 1;
                if (y < HEADER_LINES) y = HEADER_LINES;
                if (x + marqueeMessage.length() >= consoleWidth) 
                    x = std::max(0, consoleWidth - (int)marqueeMessage.length());
                
                // Clear animation area
                for (int row = HEADER_LINES; row < inputLine; ++row) {
                    setCursorPosition(0, row);
                    std::cout << std::string(consoleWidth, ' ');
                }
                
                // Draw marquee message
                setCursorPosition(x, y);
                std::cout << marqueeMessage << std::flush;
            }

            // Update position
            int consoleWidth, consoleHeight;
            getConsoleSize(consoleWidth, consoleHeight);
            int inputLine = consoleHeight - RESERVED_HISTORY_LINES - 1;
            
            x += dx;
            y += dy;
            
            if (x <= 0 || x + marqueeMessage.length() >= consoleWidth) dx = -dx;
            if (y <= HEADER_LINES || y >= inputLine - 1) dy = -dy;
            if (y >= inputLine) y = inputLine - 1;

            lastFrame = currentTime;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void MarqueeConsole::run() {
    clearScreen();
    displayMarqueeHeader();
    
    commandHistory.clear();
    marqueeRunning = true;
    
    // Start animation thread
    marqueeThread = std::thread(&MarqueeConsole::runMarqueeAnimation, this);
    
    std::string input;
    
    #ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(console, &cursorInfo);
    cursorInfo.bVisible = true;
    SetConsoleCursorInfo(console, &cursorInfo);
    #endif

    while (marqueeRunning) {
        {
            std::lock_guard<std::mutex> lock(consoleMutex);
            int consoleWidth, consoleHeight;
            getConsoleSize(consoleWidth, consoleHeight);
            int inputLine = consoleHeight - RESERVED_HISTORY_LINES - 1;
            
            for (int row = inputLine; row < consoleHeight; ++row) {
                setCursorPosition(0, row);
                std::cout << std::string(consoleWidth, ' ');
            }
            
            // command history
            int startIdx = commandHistory.size() > RESERVED_HISTORY_LINES ? 
                          commandHistory.size() - RESERVED_HISTORY_LINES : 0;
            for (int i = startIdx; i < commandHistory.size(); ++i) {
                int y = inputLine + 1 + (i - startIdx);
                setCursorPosition(0, y);
                std::cout << commandHistory[i];
            }
            
            std::string prompt = "Enter command for the MARQUEE_CONSOLE: ";
            setCursorPosition(0, inputLine);
            std::cout << prompt << input;
            setCursorPosition(static_cast<int>(prompt.size() + input.size()), inputLine);
            
            #ifdef _WIN32
            SetConsoleCursorInfo(console, &cursorInfo);
            #endif
        }
        
        #ifdef _WIN32
        if (_kbhit()) {
            char ch = _getch();
            if (ch == '\r') {
                {
                    std::lock_guard<std::mutex> lock(consoleMutex);
                    commandHistory.push_back("Command processed in MARQUEE_CONSOLE: " + input);
                    if (input == "exit") {
                        commandHistory.push_back("Exiting Marquee Console...");
                    }
                }
                if (input == "exit") {
                    marqueeRunning = false;
                }
                input.clear();
            } else if (ch == '\b' || ch == 127) {
                if (!input.empty()) input.pop_back();
            } else if (isprint(static_cast<unsigned char>(ch))) {
                input += ch;
            }
        }
        #else
        std::getline(std::cin, input);
        if (input == "exit") {
            marqueeRunning = false;
        }
        #endif
        
        std::this_thread::sleep_for(std::chrono::milliseconds(POLLING_DELAY));
    }
    
    if (marqueeThread.joinable()) {
        marqueeThread.join();
    }
    
    if (exitCallback) {
        exitCallback();
    } else {
        displayMainHeader();
    }
}

void MarqueeConsole::stop() {
    if (marqueeRunning) {
        marqueeRunning = false;
        if (marqueeThread.joinable()) {
            marqueeThread.join();
        }
    }
}

void MarqueeConsole::setMessage(const std::string& message) {
    marqueeMessage = message;
}

void MarqueeConsole::setExitCallback(ExitCallback callback) {
    exitCallback = callback;
}