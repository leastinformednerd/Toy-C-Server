from socket import *
from multiprocessing import Pool
import struct

def try_send(length):
    s = socket(AF_INET, SOCK_STREAM)
    s.connect(("192.168.68.117", 5000))

    s.send(
        struct.pack("q", length)
    )

    if length < 0:
        return

    msg = bytes(range(length))

    s.send(
        struct.pack(f"{length}s", msg)
    )

    print(s.recv(length))

with Pool(10) as p:
    p.map(try_send, [4,5,-1,7,7])
