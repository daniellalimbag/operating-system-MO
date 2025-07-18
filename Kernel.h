#pragma once

#include <string>
#include "Screen.h"

class Kernel {
public:
    void Run();
    static Kernel& Get();

    Kernel(const Kernel&) = delete;
    Kernel& operator=(const Kernel&) = delete;
private:
    Kernel();
    ~Kernel();
private:
    Screen screen;
};
