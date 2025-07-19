/*
Contributors:
Limbag, Daniella Franxene P.
Gomez, Dominic Joel M.
Iral II, John Rovere N.
Angeles, Marc Andrei D.
*/
#include "Kernel.h"
#include "ShellPrompt.h"

/**
 * @brief Entry point of the Simple OS application.
 * @details This function represents the very basic bootloader and initial
 * setup sequence of the OS, bringing up the kernel and then the shell prompt.
 */
int main() {
    Kernel kernel;                      // initialize core OS services
    ShellPrompt shellPrompt(kernel);    // Shell interacts with the initialized kernel
    shellPrompt.start();                // starts the CLI

    return 0;
}
