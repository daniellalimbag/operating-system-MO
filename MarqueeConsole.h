#ifndef MARQUEE_CONSOLE_H
#define MARQUEE_CONSOLE_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#endif

class MarqueeConsole {
private:
    std::mutex consoleMutex;
    std::vector<std::string> commandHistory;
    std::thread marqueeThread;
    std::atomic<bool> marqueeRunning = false;
    std::string marqueeMessage = "Hello world in marquee!";
    
    int x = 0, y = 3;
    int dx = 1, dy = 1;
    
    static constexpr int FRAME_DELAY = 16;  // 60 FPS
    static constexpr int POLLING_DELAY = 8; // 120 Hz polling
    static constexpr int RESERVED_HISTORY_LINES = 5;
    static constexpr int HEADER_LINES = 3;

    // Helper functions
    void setCursorPosition(int x, int y);
    void getConsoleSize(int& width, int& height);
    void displayMarqueeHeader();
    void runMarqueeAnimation();
    void clearScreen();
    void displayMainHeader();
    
public:
    MarqueeConsole();
    ~MarqueeConsole();
    
    void run();
    void stop();
    void setMessage(const std::string& message);
    
    using ExitCallback = std::function<void()>;
    void setExitCallback(ExitCallback callback);
    
private:
    ExitCallback exitCallback;
};

#endif