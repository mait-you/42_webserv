#!/usr/bin/env python3
import os

def get_fd_info(fd):
    try:
        # readlink on /proc/self/fd/N tells us what the fd points to
        link = os.readlink("/proc/self/fd/" + str(fd))
        return link
    except OSError:
        return None

def main():
    print("Content-Type: text/plain\r\n\r\n")
    print("=== OPEN FILE DESCRIPTORS IN CGI CHILD ===\n")

    # /proc/self/fd lists all open fds as symlinks
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

        # Get flags to check FD_CLOEXEC
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
        print("")

if __name__ == "__main__":
    main()
