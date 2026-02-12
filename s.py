from http.server import BaseHTTPRequestHandler, HTTPServer

class MyHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)              # HTTP status code
        self.send_header("Content-type", "text/plain")
        self.end_headers()
        self.wfile.write(b"Hello")           # send response body

def run():
    server_address = ("", 8080)             # listen on port 8080
    httpd = HTTPServer(server_address, MyHandler)
    print("Server running on port 8080...")
    httpd.serve_forever()

if __name__ == "__main__":
    run()
