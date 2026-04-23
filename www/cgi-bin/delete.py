#!/usr/bin/env python3
import os, sys, time, hashlib, html

cookie_header = os.environ.get("HTTP_COOKIE", "")
sid = None
for part in cookie_header.split(";"):
    part = part.strip()
    if part.startswith("sid="):
        sid = part[4:]; break

new_cookie = ""
if not sid:
    raw = str(time.time()) + str(os.getpid())
    sid = hashlib.sha256(raw.encode()).hexdigest()[:32]
    new_cookie = "Set-Cookie: sid={}; Path=/; Max-Age=86400; HttpOnly\r\n".format(sid)

method = os.environ.get("REQUEST_METHOD", "GET")
ua     = html.escape(os.environ.get("HTTP_USER_AGENT", ""))
host   = html.escape(os.environ.get("HTTP_HOST", ""))

sys.stdout.write("Status: 200 OK\r\n")
sys.stdout.write("Content-Type: text/html; charset=utf-8\r\n")
if new_cookie: sys.stdout.write(new_cookie)
sys.stdout.write("\r\n")

sys.stdout.write(f"""<!DOCTYPE html>
<html><head><title>Session · CGI</title>
<link rel="stylesheet" href="/assets/css/styles.css"></head>
<body>
<div class="auth-wrap"><div class="auth-card">
  <h1>Session (Python CGI)</h1>
  <p class="sub">This page was rendered by <code>session.py</code>.</p>
  <div class="notice">sid = <code>{html.escape(sid)}</code></div>
  <p class="muted">Method: {html.escape(method)}<br>Host: {host}<br>UA: {ua}</p>
  <a class="btn btn-primary btn-block" href="/dashboard/file-manager.html">Open dashboard</a>
</div></div>
</body></html>""")
