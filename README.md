# CSOPESY MP

## Overview

This is the first group machine project in CSOPESY: Process Multiplexer and CLI. It simulates a multi-core CPU scheduler and process management system, inspired by Linux's `screen` command and typical shell environments. The system supports process creation, scheduling, and execution of basic instructions, with logging and reporting features.

## Features
- Multi-core CPU scheduler (FCFS, with planned support for Round Robin)
- Process creation and management via CLI commands
- Per-process instruction execution and logging
- Real-time process status and CPU utilization reporting
- Configurable system parameters via `config.txt`

## Commands
The following commands are supported:

- `initialize`: Initialize the processor configuration from `config.txt`. **Must be run before any other command (except `exit`)**.
- `screen -s <name>`: Create a new process (screen session) with the specified name and attach to it.
- `screen -r <name>`: Reattach to a running process session by name.
- `screen -ls`: List all running and finished processes, including CPU/core usage.
- `scheduler-start`: Start generating dummy processes at the configured frequency.
- `scheduler-stop`: Stop generating dummy processes.
- `report-util`: Generate a CPU utilization report and save it to `csopesy-log.txt`.
- `clear`: Clear the console screen and refresh the header.
- `exit`: Exit the application.

## Process Instructions
Processes execute a set of pre-determined instructions, which may include:
- `PRINT(msg)`: Log a message (default: `Hello world from <process_name>!`).
- `DECLARE(var, value)`: Declare a uint16 variable.
- `ADD(var1, var2/value, var3/value)`: Addition operation.
- `SUBTRACT(var1, var2/value, var3/value)`: Subtraction operation.
- `SLEEP(X)`: Sleep for X CPU ticks.
- `FOR([instructions], repeats)`: For-loop with up to 3 levels of nesting.

Instruction types and counts are randomized per process, based on configuration.

## Configuration
Before running most commands, you must initialize the system using the `initialize` command. This reads parameters from `config.txt`, which should be present in the project directory. Example `config.txt` parameters:

```
num-cpu 4
scheduler fcfs
quantum-cycles 1
batch-process-freq 2
min-ins 10
max-ins 20
delays-per-exec 0
```

- **num-cpu**: Number of CPU cores (1-128)
- **scheduler**: Scheduling algorithm (`fcfs` or `rr`)
- **quantum-cycles**: Time slice for round-robin (if used)
- **batch-process-freq**: Frequency of process generation (in CPU cycles)
- **min-ins**: Minimum instructions per process
- **max-ins**: Maximum instructions per process
- **delays-per-exec**: Delay per instruction (in CPU cycles)

## How to Build

### A. Manually (Command Line)
1. Open a terminal in the project directory.
2. Compile the project using g++:
   ```sh
   # For Windows
   g++ -g Main.cpp -o csopesy.exe -std=c++20
   # For macOS/Linux
   g++ -g Main.cpp -o csopesy -std=c++20
   ```
   - If you add more `.cpp` files, include them in the command above.
   - On macOS, `g++` might be an alias for `clang++`

### B. In Visual Studio Code (VSCode)
1. Open the project folder in VSCode.
2. Build using Tasks:
   - Press `Ctrl+Shift+P` to open the Command Palette.
   - Type and select `Tasks: Run Task`.
   - Choose the `build csopesy` task (if available).
   - If you don't see a build task, you may need to create or update `.vscode/tasks.json` (see below).

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
4. If you don't have a launch configuration, VSCode will offer to create one. Accept and ensure your `.vscode/launch.json` looks like this:
   ```json
   {
     "version": "0.2.0",
     "configurations": [
       {
         "name": "Run csopesy.exe",
         "type": "cppvsdbg",
         "request": "launch",
         "program": "${workspaceFolder}/csopesy.exe",
         "args": [],
         "stopAtEntry": false,
         "cwd": "${workspaceFolder}",
         "environment": [],
         "console": "integratedTerminal"
       }
     ]
   }
   ```
5. Now you can use the Run/Debug controls in VSCode to start, stop, and debug your program.

## VSCode Configuration Files

- **.vscode/tasks.json** (for building):
  ```json
  {
    "version": "2.0.0",
    "tasks": [
      {
        "label": "build csopesy",
        "type": "shell",
        "command": "g++",
        "args": [
          "-g",
          "Main.cpp",
          "-o",
          "csopesy.exe"
        ],
        "group": {
          "kind": "build",
          "isDefault": true
        },
        "problemMatcher": ["$gcc"]
      }
    ]
  }
  ```
- **.vscode/launch.json** (for running/debugging):
  (see above)

**Note:**
- Make sure you have the C++ extension installed in VSCode (e.g., "C/C++" by Microsoft).
- If you use additional source files, add them to the build command and tasks.
- If you encounter issues, check your compiler installation and environment variables.

## File Structure
- `Main.cpp`: Entry point of the application.
- `Console.h`: Contains the `OpesyConsole` class, which manages the CLI and user interaction.
- `Process.h`: Defines the `Process` class and process instruction logic.
- `ProcessManager.h`: Manages process creation, lookup, and execution.
- `Scheduler.h`: Implements the CPU scheduler and worker threads.
- `Header.h`: Contains the ASCII art header for the application.
- `config.txt`: (User-provided) Configuration file for system parameters.

## Contributors
- Limbag, Daniella Franxene P.
- Gomez, Dominic Joel M.
- Iral II, John Rovere N.
- Angeles, Marc Andrei D.
