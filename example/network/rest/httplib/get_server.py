import json
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs

class SimpleGetHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        parsed_url = urlparse(self.path)
        if parsed_url.path == "/get":
            query_params = parse_qs(parsed_url.query)
            # Check the "query" parameter as in the C++ example
            query_value = query_params.get("query", [""])[0]
            response = {
                "args": {"query": query_value},
                "headers": dict(self.headers),
                "origin": self.client_address[0],
                "url": f"http://{self.server.server_address[0]}:{self.server.server_address[1]}{self.path}"
            }
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(response, indent=2).encode("utf-8"))
        else:
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Not Found")

if __name__ == "__main__":
    server_address = ("127.0.0.1", 50011)
    httpd = HTTPServer(server_address, SimpleGetHandler)
    print(f"Serving HTTP GET on {server_address[0]}:{server_address[1]}")
    httpd.serve_forever()

