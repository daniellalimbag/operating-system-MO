# CSOPESY Multiprocessor Simulator

## Overview

This is the second group machine project in CSOPESY: The Multiprocessor Simulator is a simulated multi-core operating system written in C++. It emulates core OS features including memory management, process scheduling, demand paging, and a shell-style CLI. This version builds upon the MO1 foundation by introducing memory access emulation, paging, memory visualization tools, and user-defined instruction scripting.

Inspired by tools like nvidia-smi, vmstat, and screen, this project aims to mimic fundamental OS operations

## Features
- Multi-core CPU scheduling: Supports FCFS and Round Robin schedulers with simulated process multiplexing.
- Demand Paging Memory Manager: Implements page fault handling and page replacement for limited physical memory.
- Virtual Memory Access: Processes can simulate READ and WRITE operations to memory addresses.
- Process Shell Interface: Shell-style command system (e.g., screen -s, screen -c, screen -r, scheduler-start) for creating and managing processes.
- Memory Visualization: Tools like process-smi and vmstat provide real-time memory usage and page statistics.
- Backing Store: Uses csopesy-backing-store.txt to store swapped-out pages during memory overflows.
- Process Variables: Each process has a symbol table for variables with strict size limits (max 32 variables, 64 bytes).
- Access Violations: Invalid memory accesses trigger process shutdowns with timestamped error messages.
- Configurable system parameters via `config.txt`

## Main Menu Commands
Command	   Description
-`process-smi`	Shows high-level memory usage and process allocations.
-`vmstat`	Displays detailed active/inactive process and memory stats.
-`screen -s <name> <size>`	Creates a process with the given name and memory size.
-`screen -c <name> <size> "<instructions>"`	Creates a process with a script of up to 50 instructions.
-`screen -r <name>`	Attaches to a running process screen.
-`scheduler-start`	Starts scheduling all processes using current policy.


## How to Build

### A. VSCode
1. **Open the project folder** in VSCode.
2. **Build using Tasks:**
   - Press `Ctrl+Shift+P` to open the Command Palette.
   - Type and select `Tasks: Run Task`.
   - Choose `build csopesy` from the list.
   - This will compile all project source files (`Main.cpp`, `Config.cpp`, `ProcessInstruction.cpp`, `Scheduler.cpp`, `MarqueeConsole.cpp`) into `csopesy.exe` (Windows) or `csopesy` (macOS).
3. If you add more `.cpp` files, update `.vscode/tasks.json` to include them in the `"args"` list.

You can also **build and run the project using the Run/Debug button**. This will use your `.vscode/launch.json` configuration to build and launch `csopesy.exe` automatically.

### B. Manually (Command Line)

**On Windows:**
```sh
g++ -std=c++20 -g Main.cpp Config.cpp ProcessInstruction.cpp Scheduler.cpp MarqueeConsole.cpp -o csopesy.exe
```

**On macOS/Linux:**
```sh
g++ -std=c++20 -g Main.cpp Config.cpp ProcessInstruction.cpp Scheduler.cpp MarqueeConsole.cpp -o csopesy
```

- Make sure you have a C++20-compatible compiler (`g++` 10 or later).
- If you add more `.cpp` files, include them in the command above.

If you encounter errors about missing files, ensure all `.cpp` files are listed in the build command or the VSCode task.

## How to Run

### A. Manually (Command Line)
1. After building, run:
   ```sh
   # For Windows
   .\csopesy.exe
   # For macOS/Linux
   ./csopesy
   ```

### B. In Visual Studio Code (Run/Debug Button)
1. Open `Main.cpp` or any source file.
2. Press `F5` or click the green "Run" button in the Run & Debug sidebar.
3. If prompted, select `C++ (GDB/LLDB)` or `C++ (Windows)` as the environment.
4. If you don't have a launch configuration, VSCode will offer to create one.
5. Now you can use the Run/Debug controls in VSCode to start, stop, and debug your program.

> **Note:** Make sure to use the right path to your C compiler in the launch.json file.

## Contributors
- Limbag, Daniella Franxene P.
- Gomez, Dominic Joel M.
- Iral II, John Rovere N.
- Angeles, Marc Andrei D.
