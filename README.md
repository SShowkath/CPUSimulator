# CPU Simulator Implementation Analysis

## Overview
This is a CPU simulator that implements a simple computer architecture with the following key components:

- Memory space divided into user (0-999) and system (1000-1999) segments
- Basic CPU registers: PC (Program Counter), SP (Stack Pointer), IR (Instruction Register), AC (Accumulator), X, Y
- Inter-process communication between CPU and Memory processes using pipes
- Timer-based interrupts
- User/Kernel mode switching

## Key Components

### Memory Process
- Manages a 2000-entry array (0-1999)
- Handles read/write operations from the CPU
- Divides memory into user space (0-999) and system space (1000-1999)
- Processes instructions from input files

### CPU Process
- Implements 30 different instructions (1-30) plus halt (50)
- Maintains registers: PC, SP, IR, AC, X, Y
- Handles interrupts and mode switching (user/kernel)
- Communicates with memory process via pipes

## Instruction Set
Key instructions include:

1. Memory Operations:
   - Load/Store operations (1-7)
   - Stack operations (27-28)
   
2. Register Operations:
   - Copy between registers (14-19)
   - Arithmetic operations (10-13)
   
3. Control Flow:
   - Jump instructions (20-24)
   - System calls (29-30)
   
4. I/O Operations:
   - Random number generation (8)
   - Output operations (9)

## Sample Programs

The code includes several sample programs:

1. sample1.txt: Prints the alphabet (A-Z) followed by numbers 1-10
2. sample2.txt: Creates an ASCII art face using various characters
3. sample3.txt: Repeatedly prints the letter 'A' while maintaining a counter using system calls
4. sample4.txt: Demonstrates stack operations and system calls
5. sample5.txt: Creates a pattern using character output and loops

## Protection Mechanisms

The simulator implements basic protection:
- Memory access protection (user programs cannot access system memory)
- Mode switching (user/kernel) for system calls
- Interrupt handling for timer-based context switching

## Error Handling
The system includes error checking for:
- Invalid memory access
- Incorrect number of program arguments
- File opening errors
- Pipe creation errors
- Invalid instructions

## Implementation Details
- Uses fork() to create separate CPU and memory processes
- Implements IPC using pipes for CPU-memory communication
- Supports both integer and character output modes
- Includes a timer-based interrupt system
