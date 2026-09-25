import json
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs

class DeleteRequestHandler(BaseHTTPRequestHandler):
    def do_DELETE(self):
        parsed_url = urlparse(self.path)
        if parsed_url.path == "/resource/1":
            query_params = parse_qs(parsed_url.query)
            content_length = int(self.headers.get("Content-Length", 0))
            body_data = self.rfile.read(content_length).decode("utf-8") if content_length > 0 else ""

            response_data = {
                "status": "success",
                "action": "deleted",
                "target": parsed_url.path,
                "params": query_params,
                "body": body_data
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
    server_address = ("127.0.0.1", 20015)
    httpd = HTTPServer(server_address, DeleteRequestHandler)
    print(f"Serving HTTP DELETE on {server_address[0]}:{server_address[1]}")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server.")
        httpd.server_close()

