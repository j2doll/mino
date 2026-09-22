from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse

class HeadRequestHandler(BaseHTTPRequestHandler):
    def do_HEAD(self):
        parsed_url = urlparse(self.path)
        if parsed_url.path == "/check":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", "1024")
            self.send_header("X-Resource-Status", "Available")
            self.end_headers()
            # HEAD 요청은 본문 데이터를 전송하지 않습니다.
        else:
            self.send_response(404)
            self.end_headers()

if __name__ == "__main__":
    server_address = ("127.0.0.1", 20016)
    httpd = HTTPServer(server_address, HeadRequestHandler)
    print(f"Serving HTTP HEAD on {server_address[0]}:{server_address[1]}")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server.")
        httpd.server_close()

