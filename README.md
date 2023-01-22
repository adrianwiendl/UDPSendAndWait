# UDPSendAndWait
Beleg im WS2022/2023 im Modul Rechnernetze und Kommunikationssysteme an der HTW Dresden

# Compiling programs: # 
* gcc:  
    * Attention: With the addition of inet_pton() and inet_ntop() to the source code
    gcc no longer works to compile the program.
* MSVC: 
    * Must be compiled with lws2_32 flag  
    * cl app_server.c checksum.c /Fe:..\Exe\ ws2_32.lib  
    * cl app_client.c checksum.c /Fe:..\Exe\ ws2_32.lib
# Arguments/Flags: #
* Server
    * -a=[seqNo]: Forces missing acknowledgement on packet with sequence-number [seqNo]
* Client
    * -c=[seqNo]: Falsifies checksum on packet with sequence-number [seqNo] 
    * -s=[seqNo]: Skips packet with sequence-number [seqNo]
# Usage example: #
1. Navigate two terminals to directory
2. Start Server first: .\app_server.exe "..\File\out.txt" 50000 -a=5
3. Start Client second:  .\app_client.exe "..\File\in.txt" 50000 "::1" -c=13 -s=16