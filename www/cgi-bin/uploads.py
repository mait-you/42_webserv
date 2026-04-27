#!/usr/bin/env python3

import html
import os
import re
import sys
import time
import uuid

def get_env(name, default=""):
    return os.environ.get(name, default)


def upload_dir():
    return "../uploads/"


def sanitize_filename(filename):
    base_name = os.path.basename(filename)
    if not base_name:
        base_name = "upload.bin"
    base_name = re.sub(r"[^A-Za-z0-9._-]", "_", base_name)
    if not base_name or base_name in {".", ".."}:
        base_name = "upload.bin"
    return base_name


def unique_path(directory, filename):
    safe_name = sanitize_filename(filename)
    stamp = str(int(time.time()))
    token = uuid.uuid4().hex[:8]
    return os.path.join(directory, "upload_" + stamp + "_" + token + "_" + safe_name)


def read_request_body():
    length = get_env("CONTENT_LENGTH", "0")
    if not length.isdigit():
        return b""
    return sys.stdin.buffer.read(int(length))


def parse_content_disposition(value):
    params = {}
    for chunk in value.split(";"):
        chunk = chunk.strip()
        if "=" not in chunk:
            continue
        key, raw = chunk.split("=", 1)
        key = key.strip().lower()
        raw = raw.strip()
        if len(raw) >= 2 and raw[0] == '"' and raw[-1] == '"':
            raw = raw[1:-1]
        params[key] = raw
    return params


def parse_multipart(body, boundary):
    boundary_bytes = b"--" + boundary.encode("utf-8")
    segments = body.split(boundary_bytes)
    files = []

    for segment in segments[1:-1]:
        if segment.startswith(b"\r\n"):
            segment = segment[2:]
        elif segment.startswith(b"\n"):
            segment = segment[1:]
        if segment.endswith(b"\r\n"):
            segment = segment[:-2]
        elif segment.endswith(b"\n"):
            segment = segment[:-1]
        if not segment:
            continue

        header_block, separator, content = segment.partition(b"\r\n\r\n")
        if not separator:
            continue

        headers = {}
        for raw_line in header_block.split(b"\r\n"):
            if b":" not in raw_line:
                continue
            name, value = raw_line.split(b":", 1)
            headers[name.decode("utf-8", "ignore").strip().lower()] = value.decode("utf-8", "ignore").strip()

        disposition = headers.get("content-disposition", "")
        if not disposition:
            continue

        params = parse_content_disposition(disposition)
        if params.get("name") is None:
            continue

        if content.endswith(b"\r\n"):
            content = content[:-2]
        elif content.endswith(b"\n"):
            content = content[:-1]

        files.append({
            "name": params.get("name", ""),
            "filename": params.get("filename", ""),
            "content": content,
        })

    return files


def render_page(title, body_html, status="200 OK"):
    sys.stdout.write("Status: " + status + "\r\n")
    sys.stdout.write("Content-Type: text/html; charset=utf-8\r\n\r\n")
    sys.stdout.write("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>")
    sys.stdout.write(html.escape(title))
    sys.stdout.write("</title></head><body>")
    sys.stdout.write(body_html)
    sys.stdout.write("</body></html>")


def render_form(message=""):
    body = [
        "<h1>Upload a file</h1>",
        "<form method=\"POST\" enctype=\"multipart/form-data\">",
        "<label for=\"image\">Choose a file:</label>",
        "<input type=\"file\" name=\"image\" id=\"image\" required>",
        "<button type=\"submit\">Upload</button>",
        "</form>",
    ]
    if message:
        body.insert(1, "<p>" + html.escape(message) + "</p>")
    render_page("File Upload", "".join(body))


def save_uploaded_file(files):
    field = None
    for item in files:
        if item.get("filename"):
            field = item
            break

    if field is None:
        return None, "No file was uploaded."

    directory = upload_dir()
    try:
        if not os.path.isdir(directory):
            os.makedirs(directory)
    except OSError as exc:
        return None, "Unable to create upload directory: " + str(exc)

    destination = unique_path(directory, field["filename"])
    try:
        with open(destination, "wb") as output_file:
            output_file.write(field["content"])
    except OSError as exc:
        return None, "Unable to save file: " + str(exc)

    return destination, ""


def main():
    method = get_env("REQUEST_METHOD", "GET").upper()

    if method == "GET":
        render_form()
        return

    if method != "POST":
        render_page("Unsupported Method", "<h1>405 Method Not Allowed</h1>", "405 Method Not Allowed")
        return

    content_type = get_env("CONTENT_TYPE", "")
    if "multipart/form-data" not in content_type:
        render_page("Upload Failed", "<h1>Upload failed</h1><p>Expected multipart/form-data.</p>", "400 Bad Request")
        return

    boundary_match = re.search(r"boundary=([^;]+)", content_type)
    if not boundary_match:
        render_page("Upload Failed", "<h1>Upload failed</h1><p>Missing multipart boundary.</p>", "400 Bad Request")
        return

    boundary = boundary_match.group(1).strip().strip('"')
    body = read_request_body()
    files = parse_multipart(body, boundary)
    saved_path, error = save_uploaded_file(files)

    if error:
        render_page("Upload Failed", "<h1>Upload failed</h1><p>" + html.escape(error) + "</p>", "400 Bad Request")
        return

    relative_path = os.path.relpath(saved_path, get_env("DOCUMENT_ROOT", os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))))
    body = [
        "<h1>Upload successful</h1>",
        "<p>Saved as: " + html.escape(os.path.basename(saved_path)) + "</p>",
        "<p><a href=\"/" + html.escape(relative_path).replace("\\", "/") + "\">Download file</a></p>",
        "<p><a href=\"/\">Back to home</a></p>",
    ]
    render_page("Upload Successful", "".join(body), "201 Created")


if __name__ == "__main__":
    main()