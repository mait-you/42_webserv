#!/usr/bin/env python3
"""
CGI upload helper. Reads a multipart/form-data POST from stdin and writes
files into ../uploads/. Returned as JSON.
"""
import os, sys, json, cgi, io

UPLOAD_DIR = os.path.join(os.path.dirname(__file__), "..", "uploads")
os.makedirs(UPLOAD_DIR, exist_ok=True)

length = int(os.environ.get("CONTENT_LENGTH", "0") or 0)
ctype  = os.environ.get("CONTENT_TYPE", "")
saved  = []
error  = None

try:
    fs = cgi.FieldStorage(
        fp=sys.stdin.buffer,
        environ={"REQUEST_METHOD": "POST", "CONTENT_TYPE": ctype, "CONTENT_LENGTH": str(length)},
        keep_blank_values=True,
    )
    for key in fs.keys():
        item = fs[key]
        items = item if isinstance(item, list) else [item]
        for it in items:
            if getattr(it, "filename", None):
                safe = os.path.basename(it.filename)
                dest = os.path.join(UPLOAD_DIR, safe)
                with open(dest, "wb") as f:
                    f.write(it.file.read())
                saved.append(safe)
except Exception as e:
    error = str(e)

sys.stdout.write("Status: 200 OK\r\n")
sys.stdout.write("Content-Type: application/json\r\n\r\n")
sys.stdout.write(json.dumps({"saved": saved, "error": error}))
