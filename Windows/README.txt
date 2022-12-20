Compiling programs: 
- Must be compiled with lws2_32 flag
-- gcc: 
--- Attention: With the addition of inet_pton() and inet_ntop() to the source code
    gcc no longer works to compile the program.
--- gcc -o ..\Exe\sender sender.c -lws2_32
--- gcc -o ..\Exe\receiver receiver.c -lws2_32
-- MSVC: 
--- cl sender.c /Fe:..\Exe\ ws2_32.lib
--- cl receiver.c /Fe:..\Exe\ ws2_32.lib

Usage:
- Navigate two terminals to directory
- Start receiver first: receiver.exe "..\File\out.txt" 50000
- Start sender second:  sender.exe "..File\in.txt" 50000 "::1"