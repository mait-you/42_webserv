# !/usr/bin/env python3

import os
import sys

def w(s):
    sys.stdout.write(s)

w("Content-Type: text/html\r\n\r\n")

method = os.environ.get("REQUEST_METHOD", "UNKNOWN")

w("<h1>CGI Test</h1>\r\n")
w("<p><b>Method:</b> " + method + "</p>\r\n")

w("<h2>Headers</h2>\r\n")
w("<pre>\r\n")
for key, value in os.environ.items():
    if key.startswith("HTTP_") or key in ["CONTENT_TYPE", "CONTENT_LENGTH"]:
        w(key + " = " + value + "\r\n")
w("</pre>\r\n")

if method == "POST":
    length = os.environ.get("CONTENT_LENGTH", "0")
    length = int(length) if length.isdigit() else 0
    body = sys.stdin.read(length)
    body = body.replace("\r\n", "\n").replace("\r", "\n")
    w("<h2>Body</h2>\r\n")
    w("<pre>" + body + "</pre>\r\n")
