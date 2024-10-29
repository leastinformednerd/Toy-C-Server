This is a socket-based implementation of a server that does the following with a remote connection:
1. Reads 8 bytes as a signed integer, describing the remaining length of the message
    -   If this length is negative, it is interpreted as an instruction to end the server
2. Reads that many bytes to a buffer, reverses them, and sends them back

It handles incoming connections in a safe threaded context to allow multiple connections to be serviced simultaneously and gracefully shutdown when a exit instruction is received. 
