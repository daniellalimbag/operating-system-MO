# CSOPESY Multiprocessor Simulator

## Overview

This is the first group machine project in CSOPESY: Process Multiplexer and CLI. It simulates a multi-core CPU scheduler and process management system, inspired by Linux's `screen` command and typical shell environments. The system supports process creation, scheduling, and execution of basic instructions, with logging and reporting features.

## Features
- Multi-core CPU scheduler (supports FCFS and Round Robin)
- Processes are managed and scheduled across multiple simulated CPU cores (threads)
- Process creation and management via CLI commands
- Per-process instruction execution (PRINT, DECLARE, ADD, SUBTRACT, SLEEP, FOR)
- Real-time process status and CPU utilization reporting
- Marquee Console that bounces text with dynamic borders
- Configurable system parameters via `config.txt`

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
