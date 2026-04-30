#!/usr/bin/env python3
import os
import sys
import uuid
import json

# Directory to store session data
SESSION_DIR = "sessions"

def get_cookie():
    cookie_str = os.environ.get("HTTP_COOKIE", "")
    cookies = {}
    if cookie_str:
        for item in cookie_str.split(";"):
            if "=" in item:
                k, v = item.strip().split("=", 1)
                cookies[k] = v
    return cookies

def main():
    # Ensure session directory exists
    if not os.path.exists(SESSION_DIR):
        try:
            os.makedirs(SESSION_DIR)
        except OSError:
            pass

    cookies = get_cookie()
    session_id = cookies.get("session_id")
    visits = 1
    is_new_session = False
    
    # Check if we have a valid session ID from the cookie
    if session_id and os.path.exists(os.path.join(SESSION_DIR, session_id)):
        try:
            with open(os.path.join(SESSION_DIR, session_id), "r") as f:
                data = json.load(f)
                visits = data.get("visits", 0) + 1
        except (IOError, ValueError):
            pass
    else:
        # Create a new session ID if one doesn't exist
        session_id = str(uuid.uuid4())
        is_new_session = True

    # Save session state
    try:
        with open(os.path.join(SESSION_DIR, session_id), "w") as f:
            json.dump({"visits": visits}, f)
    except IOError:
        pass

    # Strictly output \r\n endings for CGI HTTP headers
    sys.stdout.write("Status: 200 OK\r\n")
    if is_new_session:
        sys.stdout.write(f"Set-Cookie: session_id={session_id}; Path=/; HttpOnly\r\n")
    
    sys.stdout.write("Content-Type: text/html\r\n\r\n")
    
    # Output HTML content
    sys.stdout.write("<!DOCTYPE html><html><head><title>Session Management</title>")
    sys.stdout.write("<link rel='stylesheet' href='../css/style.css'>")
    sys.stdout.write("</head><body><div class='box' style='max-width: 500px;'>")
    sys.stdout.write("<h2>Session Management Test</h2>")
    
    if is_new_session:
        sys.stdout.write("<p style='color: #4CAF50; font-weight: bold;'>Welcome! A new session has been created for you.</p>")
    else:
        sys.stdout.write("<p style='color: #0066cc; font-weight: bold;'>Welcome back! Your session cookie was recognized.</p>")
    
    sys.stdout.write(f"<p style='word-break: break-all;'><strong>Session ID:</strong> {session_id}</p>")
    sys.stdout.write(f"<p><strong>Number of visits:</strong> {visits}</p>")
    
    sys.stdout.write("<div style='margin-top: 20px;'>")
    sys.stdout.write("<a href='/cgi-bin/session.py' style='display: inline-block; background: #0066cc; color: white; padding: 10px 15px; text-decoration: none; border-radius: 5px; margin-right: 10px;'>Reload Page</a>")
    sys.stdout.write("<a href='/' style='display: inline-block; padding: 10px 15px; color: #333; text-decoration: none;'>Back to Home</a>")
    sys.stdout.write("</div>")
    sys.stdout.write("</div></body></html>")

if __name__ == "__main__":
    main()
