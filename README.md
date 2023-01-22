# UDPSendAndWait
Beleg im WS2022/2023 im Modul Rechnernetze und Kommunikationssysteme

Compiling programs: 
- Must be compiled with lws2_32 flag
-- gcc: 
--- Attention: With the addition of inet_pton() and inet_ntop() to the source code
    gcc no longer works to compile the program.
-- MSVC: 
--- cl app_server.c checksum.c /Fe:..\Exe\ ws2_32.lib
--- cl app_client.c checksum.c /Fe:..\Exe\ ws2_32.lib

Usage:
- Navigate two terminals to directory
- Start Server first: .\app_server.exe "..\File\out.txt" 50000 -a=5
- Start Client second:  .\app_client.exe "..\File\in.txt" 50000 "::1" -c=13 -s=16