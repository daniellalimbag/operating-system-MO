#include "Kernel.h"
#include "Screen.h"

void Kernel::Run() {
    bool running = true;
    while (running) {
        running = screen.Run();
    }
}

// The Singleton Get method definition (Meyers Singleton)
Kernel& Kernel::Get() {
    static Kernel instance; // The single instance is created here, on first call, thread-safely
    return instance;
}

Kernel::Kernel()
{
}

Kernel::~Kernel() {
}
