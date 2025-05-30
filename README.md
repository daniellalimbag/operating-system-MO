# CSOPESY MP

Note from Dani: Guys if you are working on indiv assignments/seatwork don't push it here. Just save locally

## Overview
This is the first group machine project in CSOPESY: Process Multiplexer and CLI.

## Commands
The following commands are supported currently:

- `screen -s <name>`: Create a new screen session with the specified name.
- `screen -r <name>`: Resume an existing screen session.
- `screen -ls`: List all active screen sessions.
- `clear`: Clear the console screen and refresh the header.
- `exit`: Exit the application.

## How to Build
To build the project, use the provided build task in Visual Studio Code:

1. Open the command palette (`Ctrl+Shift+P`) and select `Tasks: Run Task`.
2. Choose the `build csopesy` task to compile the project.

Alternatively, you can compile the project manually using the following command:

```bash
powershell.exe g++ -g Main.cpp -o csopesy.exe
```

## How to Run
After building the project, run the executable:

```bash
powershell.exe .\csopesy.exe
```

## File Structure
- `Main.cpp`: Entry point of the application.
- `Console.h`: Contains the `OpesyConsole` class, which manages the CLI.
- `Screen.h`: Defines the `Screen` class for managing individual screen sessions.
- `Header.h`: Contains the ASCII art header for the application.

## Contributors
- Limbag, Daniella Franxene P.
- Gomez, Dominic Joel M.
- Iral II, John Rovere N.
- Angeles, Marc Andrei D.
