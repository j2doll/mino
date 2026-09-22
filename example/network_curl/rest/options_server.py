import json
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse

class OptionsRequestHandler(BaseHTTPRequestHandler):
    def do_OPTIONS(self):
        parsed_url = urlparse(self.path)
        if parsed_url.path == "/api":
            allowed_methods = "GET, POST, PUT, PATCH, DELETE, HEAD, OPTIONS, TRACE"
            response_data = {
                "allowed_methods": allowed_methods.split(", "),
                "endpoint": "/api"
            }
            response_body = json.dumps(response_data, indent=2).encode("utf-8")

            self.send_response(200)
            self.send_header("Allow", allowed_methods)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(response_body)))
            self.end_headers()
            self.wfile.write(response_body)
        else:
            self.send_error_response(404, "Not Found")

    def send_error_response(self, status_code, message):
        response_data = {"error": message}
        response_body = json.dumps(response_data).encode("utf-8")
        self.send_response(status_code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(response_body)))
        self.end_headers()
        self.wfile.write(response_body)

if __name__ == "__main__":
    server_address = ("127.0.0.1", 20017)
    httpd = HTTPServer(server_address, OptionsRequestHandler)
    print(f"Serving HTTP OPTIONS on {server_address[0]}:{server_address[1]}")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server.")
        httpd.server_close()

