Compiling programs: 
- Must be compiled with lws2_32 flag
-- gcc: gcc -o name name.c -lws2_32
-- MSVC: cl name.c ws2_32.lib

Usage:
- Navigate two terminals to directory
- Start receiver first: receiver.exe ["out.txt"] [69696]
- Start sender second:  sender.exe ["in.txt"] [69696] ["127.0.0.1"]