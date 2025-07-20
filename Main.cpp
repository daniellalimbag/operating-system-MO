/*
Contributors:
Limbag, Daniella Franxene P.
Gomez, Dominic Joel M.
Iral II, John Rovere N.
Angeles, Marc Andrei D.
*/
#include "Kernel.h"
#include "ShellPrompt.h"
#include <thread>

/**
 * @brief Entry point of the Simple OS application.
 * @details This function represents the very basic bootloader and initial
 * setup sequence of the OS, bringing up the kernel and then the shell prompt.
 */
int main() {
    Kernel kernel;                                      // initialize core OS services
    std::thread kernelThread(&Kernel::run, &kernel);    // start the main kernel background thread

    ShellPrompt shellPrompt(kernel);                                    // Shell interacts with the initialized kernel
    std::thread shellPromptThread(&ShellPrompt::start, &shellPrompt);   // starts the CLI thread

    shellPromptThread.join();       // wait until user types 'exit'
    kernel.shutdown();              // signal the kernel to shutdown
    kernelThread.join();            // wait for the kernel to gracefully stop

    return 0;
}
