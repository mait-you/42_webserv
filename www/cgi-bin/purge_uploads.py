#!/usr/bin/env python3
"""
cgi-bin/purge_uploads.py
GET /cgi-bin/purge_uploads.py

Deletes every file inside the uploads/ directory that sits next to www/.
Skips .gitkeep and any subdirectories.
On success  → 302 redirect to /dashboard.html
On failure  → 302 redirect to /error_pages/500.html
"""

import os
import sys

SCRIPT_DIR  = os.path.dirname(os.path.abspath(__file__))
UPLOADS_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, '..', 'uploads'))

def redirect(location):
    sys.stdout.write('Location: ' + location + '\r\n')
    sys.stdout.write('\r\n')

def main():
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
