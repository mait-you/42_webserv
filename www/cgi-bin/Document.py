#!/usr/bin/env python3
import sys
sys.stdout.buffer.write(
    b"Content-Type: text/html\r\n"
    b"\r\n"
    b"<h1>Hello from CGI</h1>"
)
