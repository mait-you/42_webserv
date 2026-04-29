#!/usr/bin/env python3
import sys
sys.stdout.buffer.write(
    b"Status: 302 Found\r\n"
    b"Location: https://example.com\r\n"
    b"Content-Type: text/html\r\n"
    b"\r\n"
    b"<h1>Redirecting...</h1>"
)
