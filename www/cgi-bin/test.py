#!/usr/bin/env python3
import os
import sys

method = os.environ.get("REQUEST_METHOD", "")
query  = os.environ.get("QUERY_STRING", "")
body   = ""

if method == "POST":
    length = os.environ.get("CONTENT_LENGTH", "0")
    if length.isdigit() and int(length) > 0:
        body = sys.stdin.read(int(length))

print("Content-Type: text/html")
print("")
print("<html><body>")
print("<h1>CGI Test Page</h1>")
print("<p>METHOD: " + method + "</p>")
if query:
    print("<p>QUERY_STRING: " + query + "</p>")
if body:
    print("<p>BODY: " + body + "</p>")
print("</body></html>")
