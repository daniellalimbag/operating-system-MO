#include <unordered_map>
#include <string>

#include "Screen.h"
#include "ConsoleIO.h"

namespace {
    const std::unordered_map<std::string_view, Screen::Command> commandMap = {
        {"exit", Screen::Command::Exit},
        {"initialize", Screen::Command::Initialize},
        {"screen", Screen::Command::Screen},
        {"scheduler-start", Screen::Command::SchedulerStart},
        {"scheduler-stop", Screen::Command::SchedulerStop},
        {"report-util", Screen::Command::ReportUtil},
        {"clear", Screen::Command::Clear}
    };
}

bool Screen::Run() {
    PrintHeader();
    return HandleInput();
}

void Screen::PrintHeader() {
    ConsoleIO::ClearScreen();
    ConsoleIO::PrintHeader();
    ConsoleIO::DrawHorizontalLine();
    ConsoleIO::PrintLine("Hello! Welcome to CSOPESY commandline!");
    ConsoleIO::PrintLine("");
    ConsoleIO::PrintLine("Type 'initialize' to start the emulator. Type 'exit' to terminate the console.");
    ConsoleIO::DrawHorizontalLine();
}

bool Screen::HandleInput() {
    bool keepRunning = true;
    std::string_view input;
    do{
        input = ConsoleIO::GetInput("root :\\>");
        if(commandMap.count(input))
            keepRunning = RunCommand(commandMap.at(input));
        else
            ConsoleIO::PrintLine("Invalid command!");
    }while(keepRunning);
    return false;
}

bool Screen::RunCommand(Command command) {
    switch(command) {
        case Command::Exit:
            ConsoleIO::PrintLine("Exit command recognized. Doing something");
            return false;
            break;
        case Command::Initialize:
            ConsoleIO::PrintLine("Initialize command recognized. Doing something");
            break;
        case Command::Screen:
            ConsoleIO::PrintLine("Screen command recognized. Doing something");
            break;
        case Command::SchedulerStart:
            ConsoleIO::PrintLine("SchedulerStart command recognized. Doing something");
            break;
        case Command::SchedulerStop:
            ConsoleIO::PrintLine("SchedulerStop command recognized. Doing something");
            break;
        case Command::ReportUtil:
            ConsoleIO::PrintLine("ReportUtil command recognized. Doing something");
            break;
        case Command::Clear:
            ConsoleIO::PrintLine("Clear command recognized. Doing something");
            ConsoleIO::ClearScreen();
            break;
    }
    return true;
}
