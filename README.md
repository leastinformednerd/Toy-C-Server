This is a socket-based implementation of a server that does the following with a remote connection:
1. Reads 8 bytes as a signed integer, describing the remaining length of the message
2. Reads that many bytes to a buffer, reverses them, and sends them back

It handles incoming connections in safe threaded contexts
