#!/usr/bin/env python3
import sys
import os

# read body from stdin
content_length = int(os.environ.get("CONTENT_LENGTH", 0))
body = sys.stdin.read(content_length) if content_length > 0 else ""

# CGI response — headers first, then blank line, then body
print("Content-Type: text/html")
print("Status: 200")
print("")
print("<html>")
print("<body>")
print("<h1>CGI POST works!</h1>")
print("<p>Method: " + os.environ.get("REQUEST_METHOD", "") + "</p>")
print("<p>Body: " + body + "</p>")
print("</body>")
print("</html>")
