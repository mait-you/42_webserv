#!/usr/bin/env python3

import socket
import time
import sys

HOST  = "127.0.0.1"
PORT  = 8080
COUNT = 20

def main():
    if len(sys.argv) == 4:
        HOST  = sys.argv[1]
        PORT  = int(sys.argv[2])
        COUNT = int(sys.argv[3])
    elif len(sys.argv) != 1:
        print(f"Usage: {sys.argv[0]} [host port count]")
        sys.exit(1)

    sockets = []

    print(f"Creating {COUNT} idle clients on {HOST}:{PORT} ...")

    for i in range(COUNT):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((HOST, PORT))
            sockets.append(s)
            print(f"  [{i+1}] connected")
        except Exception as e:
            print(f"  [{i+1}] failed: {e}")

    print(f"\n{len(sockets)} clients connected. They are now idle.")
    print("Press Ctrl+C to disconnect all.\n")

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nDisconnecting all clients ...")
        for s in sockets:
            s.close()
        print("Done.")

if __name__ == "__main__":
    main()
