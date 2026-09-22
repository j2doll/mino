import json
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse

class TraceRequestHandler(BaseHTTPRequestHandler):
    def do_TRACE(self):
        parsed_url = urlparse(self.path)
        if parsed_url.path == "/trace":
            response_data = {
                "trace_status": "echoed",
                "request_line": f"TRACE {self.path} {self.request_version}",
                "headers": dict(self.headers)
            }
            response_body = json.dumps(response_data, indent=2).encode("utf-8")

            self.send_response(200)
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
    server_address = ("127.0.0.1", 20018)
    httpd = HTTPServer(server_address, TraceRequestHandler)
    print(f"Serving HTTP TRACE on {server_address[0]}:{server_address[1]}")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server.")
        httpd.server_close()

