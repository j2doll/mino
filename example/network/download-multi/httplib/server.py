import io
import time
from http.server import HTTPServer, BaseHTTPRequestHandler

BOUNDARY = "----WebKitFormBoundary7MA4YWxkTrZu0gW" # Multipart boundary string
HOST = "127.0.0.1" # Server host
PORT = 8080 # Server port

class MultipartDownloadHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == "/download":
            self.serve_multipart()
        else:
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Not Found")

    def serve_multipart(self):
        # 전송할 테스트 파일 목록 (파일명, 내용)
        files = [
            ("file1.txt", "Hello! This is the first text file.\n".encode("utf-8")),
            ("sample.json", '{\n  "name": "test",\n  "status": 200\n}\n'.encode("utf-8")),
            ("binary_data.bin", bytes(range(256))),
        ]

        # Multipart body 조립
        body_stream = io.BytesIO()
        for filename, content in files:
            body_stream.write(f"--{BOUNDARY}\r\n".encode("utf-8"))
            body_stream.write(
                f'Content-Disposition: form-data; name="files"; filename="{filename}"\r\n'.encode("utf-8")
            )
            body_stream.write(b"Content-Type: application/octet-stream\r\n\r\n")
            body_stream.write(content)
            body_stream.write(b"\r\n")
        body_stream.write(f"--{BOUNDARY}--\r\n".encode("utf-8"))

        body_bytes = body_stream.getvalue()

        # HTTP 헤더 전송
        self.send_response(200)
        self.send_header(
            "Content-Type", f'multipart/form-data; boundary="{BOUNDARY}"'
        )
        self.send_header("Content-Length", str(len(body_bytes)))
        self.end_headers()

        # 진행률 콜백 테스트를 위해 작은 청크 단위로 나누어 전송
        chunk_size = 64
        for offset in range(0, len(body_bytes), chunk_size):
            chunk = body_bytes[offset : offset + chunk_size]
            self.wfile.write(chunk)
            self.wfile.flush()
            time.sleep(0.01)

        print(f"[Server] Transfer completed: {len(body_bytes)} bytes sent.")


def run():
    server_address = (HOST, PORT)
    httpd = HTTPServer(server_address, MultipartDownloadHandler)
    print(f"[Server] Test server started at http://{HOST}:{PORT}/download")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n[Server] Shutting down server...")
        httpd.server_close()


if __name__ == "__main__":
    run()
