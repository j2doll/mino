from http.server import BaseHTTPRequestHandler, HTTPServer

class ConnectRequestHandler(BaseHTTPRequestHandler):
    def do_CONNECT(self):
        response_body = b"Connection Established\n"
        self.send_response(200, "Connection Established")
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(response_body)))
        self.end_headers()
        self.wfile.write(response_body)

    def do_GET(self):
        response_body = b"HTTP CONNECT Mock Server Ready."
        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(response_body)))
        self.end_headers()
        self.wfile.write(response_body)

if __name__ == "__main__":
    server_address = ("127.0.0.1", 20019)
    httpd = HTTPServer(server_address, ConnectRequestHandler)
    print(f"Serving HTTP CONNECT on {server_address[0]}:{server_address[1]}")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server.")
        httpd.server_close()

