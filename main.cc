#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>

using namespace std;

int main(int args, char *argv[])
{
    if (args != 3)
    {
        cout << "ERROR: Incorrect number of arguments." << endl;
        return 0;
    }

    srand(time(0));

    // Creating pipes between cpu and memory
    int ctom[2];
    int mtoc[2];

    // Checks to see if pipes opened properly
    if (pipe(ctom) == -1 || pipe(mtoc) == -1)
    {
        cout << "An error ocurred with opening the pipes." << endl;
        return 0;
    }

    // Get filename from main parameter
    string fileName = argv[1];

    ifstream file(fileName);

    // Check to see if file properly opened
    if (!file.is_open())
    {
        cout << "Error: " << fileName << " not found" << endl;
        return 0;
    }

    // Forking processes
    int pid = fork();

    if (pid < 0)
    {
        cout << "Invalid process" << endl;
        exit(1);
    }

    // Memory process
    if (pid == 0)
    {
        // Memory integer entries
        int entries[2000];

        // User and system entry starts
        int user = 0;
        int system = 1000;

        int curPointer = user;

        // Store input file instructions into memory
        string line;
        while (getline(file, line))
        {
            int temp;
            if (line[0] == '.')
            {
                temp = stoi(line.substr(1));
                curPointer = temp;
            }
            else if (line != "")
            {
                if (line.find("//") == string::npos || line.substr(0, line.find("//")).find_first_not_of(' ') != string::npos)
                {
                    temp = stoi(line);
                    entries[curPointer] = temp;
                    curPointer++;
                }
            }
        }

        // Check for CPU Instructions
        while (true)
        {
            char input[6] = {'\0'};

            // Get CPU instructions
            read(ctom[0], &input, sizeof(input));
            char command = input[0];

            for (int i = 0; i < 5; i++)
                input[i] = input[i + 1];

            int val;

            string in(input);
            if (in.length() > 0)
                val = stoi(in);

            // Read operation
            if (command == 'r')
            {
                int index = entries[val];
                char i[6] = {'\0'};
                strcpy(i, to_string(index).c_str());
                write(mtoc[1], &i, sizeof(i));
            }
            // Write operation
            if (command == 'w')
            {
                char element[6] = {'\0'};
                read(ctom[0], &element, sizeof(element));
                string elem(element);
                entries[val] = stoi(elem);
            }

            // Exit
            if (command == 'e')
            {
                exit(0);
            }
        }
    }

    // CPU process
    else
    {
        // Get timer
        int timer = stoi(argv[2]);

        // User and system entry starts
        int userTop = 999;
        int systemTop = 1999;

        // Registers and initializing default values
        int pc = 0;
        int sp = userTop;
        int ir;
        int ac;
        int x;
        int y;

        // Interrupts
        int instructionCount = 0;
        bool user = true;
        bool kernel = false;
        int numInterrupts = 0;

        int count = 0;

        while (true)
        {
            char instr[6] = {'\0'};
            char memCall[6] = {'\0'};

            // Check if program jumped
            bool hasJumped = false;

            // Check for nested inerrupts
            if (kernel && instructionCount > 0 && (instructionCount % timer) == 0)
            {

                numInterrupts++;
            }
            // Check for timer interrupt
            if ((!kernel && instructionCount > 0 && (instructionCount % timer) == 0) || (!kernel && numInterrupts > 0))
            {

                // Switch to kernel mode
                user = false;
                kernel = true;

                // Decrement number of interrupts
                numInterrupts--;

                // Save stack values
                int userSp = sp;
                int userPc = pc;

                // Set sp to top of system stack and set pc to 1000
                sp = systemTop;
                pc = 1000;
                hasJumped = true;

                // Write user sp to memory sp
                strcpy(memCall, ("w" + to_string(sp)).c_str());
                write(ctom[1], &memCall, sizeof(memCall));
                strcpy(memCall, (to_string(userSp)).c_str());
                write(ctom[1], &memCall, sizeof(memCall));
                sp--;

                // Write user pc to memory pc
                strcpy(memCall, ("w" + to_string(sp)).c_str());
                write(ctom[1], &memCall, sizeof(memCall));
                strcpy(memCall, (to_string(userPc)).c_str());
                write(ctom[1], &memCall, sizeof(memCall));
                sp--;

                continue;
            }
            // Fetch instruction from memory

            strcpy(memCall, ("r" + to_string(pc)).c_str());

            write(ctom[1], &memCall, sizeof(memCall));
            read(mtoc[0], &instr, sizeof(instr));
            instructionCount++;

            string ins(instr);

            // Check which instruction to run if instruction is returned
            if (ins != "")
            {
                // Load instruction into ir register
                ir = stoi(ins);

                switch (ir)
                {
                // Load the value into the AC
                case 1:
                {

                    pc++;
                    char value[6] = {'\0'};

                    strcpy(memCall, ("r" + to_string(pc)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &value, sizeof(value));
                    string val(value);
                    ac = stoi(val);
                    break;
                }

                // Load the value at the address into the AC
                case 2:
                {
                    char address[6] = {'\0'};
                    pc++;

                    // Read next line for address
                    strcpy(memCall, ("r" + to_string(pc)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &address, sizeof(address));
                    string ad(address);
                    // Check for valid address
                    if (stoi(ad) > 999 && user)
                    {
                        cout << "Error: can not access system address " << address << " in user mode." << endl;
                    }
                    // Load address value into the AC
                    else
                    {
                        char value[6] = {'\0'};
                        strcpy(memCall, ("r" + ad).c_str());
                        write(ctom[1], &memCall, sizeof(memCall));
                        read(mtoc[0], &value, sizeof(value));
                        string val(value);
                        ac = stoi(val);
                    }
                    break;
                }

                // Load the value from the address found in the given address into the AC
                case 3:
                {
                    char address[6] = {'\0'};
                    pc++;

                    strcpy(memCall, ("r" + to_string(pc)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &address, sizeof(address));

                    string ad(address);
                    // Check for valid address
                    if (stoi(ad) > 999 && user)
                    {
                        cout << "Error: can not access system address " << address << " in user mode." << endl;
                    }

                    else
                    {
                        // Load address value into the AC
                        char value[6] = {'\0'};
                        strcpy(memCall, ("r" + ad).c_str());
                        write(ctom[1], &memCall, sizeof(memCall));
                        read(mtoc[0], &value, sizeof(value));
                        string val(value);
                        ac = stoi(val);

                        // Check for valid address
                        if (stoi(value) > 999 && user)
                        {
                            cout << "Error: can not access system address " << address << " in user mode." << endl;
                        }
                        // Load address value into the AC
                        else
                        {
                            strcpy(memCall, ("r" + to_string(ac)).c_str());
                            write(ctom[1], &memCall, sizeof(memCall));
                            read(mtoc[0], &value, sizeof(value));
                            string val1(val);
                            ac = stoi(val1);
                        }
                    }

                    break;
                }

                // Load the value at (address+X) into the AC
                case 4:
                {
                    char address[6] = {'\0'};
                    pc++;

                    strcpy(memCall, ("r" + to_string(pc)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &address, sizeof(address));

                    string ad(address);
                    // Check for valid address
                    if (stoi(ad) + x > 999 && user)
                    {
                        cout << "Error: can not access system address " << address << " in user mode." << endl;
                    }
                    // Load address value + x into the AC
                    else
                    {
                        char value[6] = {'\0'};
                        strcpy(memCall, ("r" + to_string((x + stoi(ad)))).c_str());
                        write(ctom[1], &memCall, sizeof(memCall));
                        read(mtoc[0], &value, sizeof(value));
                        string val(value);
                        ac = stoi(val);
                    }

                    break;
                }

                // Load the value at (address+Y) into the AC
                case 5:
                {
                    char address[6] = {'\0'};
                    pc++;

                    strcpy(memCall, ("r" + to_string(pc)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &address, sizeof(address));

                    string ad(address);
                    // Check for valid address
                    if (stoi(ad) + y > 999 && user)
                    {
                        cout << "Error: can not access system address " << address << " in user mode." << endl;
                    }
                    // Load address value + y into the AC
                    else
                    {
                        char value[6] = {'\0'};
                        strcpy(memCall, ("r" + to_string((y + stoi(ad)))).c_str());
                        write(ctom[1], &memCall, sizeof(memCall));
                        read(mtoc[0], &value, sizeof(value));
                        string val(value);
                        ac = stoi(val);
                    }

                    break;
                }

                // Load from (Sp+X) into the AC
                case 6:
                {
                    char address[6] = {'\0'};
                    string ad(address);
                    // Check for valid address
                    if (x + sp > 999 && user)
                    {
                        cout << "Error: can not access system address " << ad << " in user mode." << endl;
                    }
                    // Load address value + sp into the AC
                    else
                    {
                        char value[6] = {'\0'};
                        strcpy(memCall, ("r" + to_string(sp + x + 1)).c_str());
                        write(ctom[1], &memCall, sizeof(memCall));
                        read(mtoc[0], &value, sizeof(value));
                        string val(value);
                        ac = stoi(val);
                    }

                    break;
                }

                // Store the value in the AC into the address
                case 7:
                {
                    char address[6] = {'\0'};
                    pc++;

                    // Read next line for address
                    strcpy(memCall, ("r" + to_string(pc)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &address, sizeof(address));

                    // Write AC value into memory at the address
                    string addr(address);
                    strcpy(memCall, ("w" + addr).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    strcpy(memCall, (to_string(ac)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));

                    break;
                }

                // Gets a random int from 1 to 100 into the AC
                case 8:
                {
                    ac = 1 + rand() % 100;
                    break;
                }

                // Write AC as an int/char depending on port
                case 9:
                {
                    char address[6] = {'\0'};
                    pc++;

                    // Read next line for address
                    strcpy(memCall, ("r" + to_string(pc)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &address, sizeof(address));

                    string ad(address);
                    if (stoi(ad) == 1)
                    {
                        printf("%d", ac);
                    }

                    if (stoi(ad) == 2)
                    {
                        printf("%c", ac);
                    }

                    break;
                }

                // Add the value in X to the AC
                case 10:
                {
                    ac += x;
                    break;
                }

                // Add the value in Y to the AC
                case 11:
                {
                    ac += y;
                    break;
                }

                // Subtract the value in X from the AC
                case 12:
                {
                    ac -= x;
                    break;
                }

                // Subtract the value in Y from the AC
                case 13:
                {
                    ac -= y;
                    break;
                }

                // Copy the value in the AC to X
                case 14:
                {
                    x = ac;
                    break;
                }

                // Copy the value in X to the AC
                case 15:
                {
                    ac = x;
                    break;
                }

                // Copy the value in the AC to Y
                case 16:
                {
                    y = ac;
                    break;
                }

                // Copy the value in Y to the AC
                case 17:
                {
                    ac = y;
                    break;
                }

                // Copy the value in AC to the SP
                case 18:
                {
                    sp = ac;
                    break;
                }

                // Copy the value in SP to the AC
                case 19:
                {
                    ac = sp;
                    break;
                }

                // Jump to the address
                case 20:
                {
                    char address[6] = {'\0'};
                    pc++;

                    // Read next line for address
                    strcpy(memCall, ("r" + to_string(pc)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &address, sizeof(address));

                    string ad(address);
                    pc = stoi(ad);
                    hasJumped = true;

                    break;
                }

                // Jump to the address only if the value in the AC is zero
                case 21:
                {

                    char address[6] = {'\0'};
                    pc++;

                    // Read next line for address
                    strcpy(memCall, ("r" + to_string(pc)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &address, sizeof(address));

                    if (ac == 0)
                    {
                        string ad(address);
                        pc = stoi(ad);
                        hasJumped = true;
                    }

                    break;
                }

                // Jump to the address only if the value in the AC is not zero
                case 22:
                {
                    char address[6] = {'\0'};
                    pc++;

                    // Read next line for address

                    strcpy(memCall, ("r" + to_string(pc)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &address, sizeof(address));

                    // Jump to address
                    if (ac != 0)
                    {
                        string ad(address);
                        pc = stoi(ad);
                        hasJumped = true;
                    }

                    break;
                }

                // Push return address onto stack, jump to the address
                case 23:
                {

                    char address[6] = {'\0'};
                    pc++;

                    // Read next line for address
                    strcpy(memCall, ("r" + to_string(pc)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &address, sizeof(address));

                    // Push value onto the stack
                    strcpy(memCall, ("w" + to_string(sp)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    string ad(address);
                    strcpy(memCall, to_string(pc + 1).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    sp--;

                    // Jump to address
                    string ad1(address);
                    pc = stoi(ad1);
                    hasJumped = true;

                    break;
                }

                // Pop return address from the stack, jump to the address
                case 24:
                {
                    char address[6] = {'\0'};
                    sp++;

                    // Read top of the stack
                    strcpy(memCall, ("r" + to_string(sp)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &address, sizeof(address));

                    string ad(address);
                    pc = stoi(ad);
                    hasJumped = true;

                    break;
                }

                // Increment the value in X
                case 25:
                {
                    x++;
                    break;
                }

                // Decrement the value in X
                case 26:
                {
                    x--;
                    break;
                }

                // Push AC onto stack
                case 27:
                {
                    // Write the value in ac into the top of the stack
                    strcpy(memCall, ("w" + to_string(sp)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    strcpy(memCall, (to_string(ac)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    sp--;

                    break;
                }

                // Pop from stack into AC
                case 28:
                {
                    char value[6] = {'\0'};
                    sp++;
                    strcpy(memCall, ("r" + to_string(sp)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &value, sizeof(value));

                    string val(value);
                    ac = stoi(val);

                    break;
                }

                // Perform system call
                case 29:
                {
                    // Switch to kernel mode
                    user = false;
                    kernel = true;

                    // Store user pc and sp values
                    int userSp = sp;
                    int userPc = pc + 1;

                    // Set sp to system stack and set pc to 1500
                    sp = systemTop;
                    pc = 1500;
                    hasJumped = true;

                    // Write user sp to memory sp
                    strcpy(memCall, ("w" + to_string(sp)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    strcpy(memCall, (to_string(userSp)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    sp--;

                    // Write user pc to memory pc
                    strcpy(memCall, ("w" + to_string(sp)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    strcpy(memCall, (to_string(userPc)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    sp--;

                    break;
                }

                // Return from system call
                case 30:
                {
                    // Switch to user mode
                    user = true;
                    kernel = false;

                    char address[6] = {'\0'};
                    sp++;

                    // Read stack value
                    strcpy(memCall, ("r" + to_string(sp)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &address, sizeof(address));

                    // Store address into pc
                    string ad(address);
                    pc = stoi(ad);
                    hasJumped = true;

                    sp++;
                    // Read stack value
                    strcpy(memCall, ("r" + to_string(sp)).c_str());
                    write(ctom[1], &memCall, sizeof(memCall));
                    read(mtoc[0], &address, sizeof(address));

                    // Store value into sp
                    string ad2(address);
                    sp = stoi(ad2);

                    break;
                }

                // End execution
                case 50:
                {

                    write(ctom[1], "e", sizeof("e"));

                    // Close  pipes
                    close(ctom[0]);
                    close(ctom[1]);
                    close(mtoc[0]);
                    close(mtoc[1]);

                    return 0;

                    break;
                }

                default:
                    cout << "Invalid input: " << ir << endl;
                }

                // Go to next instruction if no jumps ocurred
                if (!hasJumped)
                {
                    pc++;
                }
            }
        }
    }
}