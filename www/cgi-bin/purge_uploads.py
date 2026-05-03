#!/usr/bin/env python3
"""
cgi-bin/purge_uploads.py
POST /cgi-bin/purge_uploads.py

Deletes every file inside the uploads/ directory that sits next to www/.
Skips .gitkeep and any subdirectories.
On success  → 302 redirect to /dashboard.html
On failure  → 302 redirect to /error_pages/500.html
"""

import os

SCRIPT_DIR   = os.path.dirname(os.path.abspath(__file__))
UPLOADS_DIR  = os.path.join(SCRIPT_DIR, '..', 'uploads')
UPLOADS_DIR  = os.path.normpath(UPLOADS_DIR)

def redirect(location):
    print('Status: 302 Found')
    print('Location: ' + location)
    print('Content-Type: text/html')
    print()
    print('<html><body>Redirecting...</body></html>')

def main():
    method = os.environ.get('REQUEST_METHOD', 'GET').upper()
    if method != 'POST':
        redirect('/error_pages/405.html')
        return

    if not os.path.isdir(UPLOADS_DIR):
        redirect('/error_pages/500.html')
        return

    try:
        for entry in os.listdir(UPLOADS_DIR):
            if entry == '.gitkeep':
                continue
            full_path = os.path.join(UPLOADS_DIR, entry)
            if os.path.isfile(full_path):
                os.remove(full_path)
    except Exception:
        redirect('/error_pages/500.html')
        return

    redirect('/dashboard.html')

if __name__ == '__main__':
    main()
