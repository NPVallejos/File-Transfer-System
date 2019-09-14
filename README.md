# File-Transfer-System

# Completion status
This program can successfully read and write binary files (i.e. .txt, .mp4, .bin, etc)
<br/>This program can successfully read files of any size (Fixed seg fault issue by not storing packets in an array of Packet's)

# My code
nicholasvallejos_server.c - all server-side code
<br/>nicholasvallejos_client.c - all client-side code
<br/>packet.h - header file that includes two structs (Header and Packet) and a macro called MAX. This file is used by server.c and <br/>client.c (more info in the file).

# How to compile code
<br/>Simply run "make" and two executables will be made: server & client

# How to run the code
<br/>Run ./server [port #]
<br/>The server hostname will be displayed in terminal
<br/>Then run ./client [filename] [serverhostname] [port #]

# BEFORE RUNNING THE CODE 
***WARNING***
<br/>Make sure that the file being read by the client is not in the same directory as the "server" executable.
<br/>Otherwise you will be trying to read a file that is also being written to, resulting in an empty file.
