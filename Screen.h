#pragma once
#include <string>

class Screen {
public:
    enum class Command
    {
        Exit,
        Initialize,
        Screen,
        SchedulerStart,
        SchedulerStop,
        ReportUtil,
        Clear,
    };
    bool Run();

private:
    void PrintHeader();
    bool HandleInput();
    bool RunCommand(Command command);
};
