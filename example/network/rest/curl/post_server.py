from http.server import BaseHTTPRequestHandler, HTTPServer
import json

class MyRequestHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        # '/post' 엔드포인트 요청인지 확인
        if self.path == '/post':
            try:
                # Content-Length 헤더를 통해 요청 본문(body)의 크기를 읽음
                content_length = int(self.headers.get('Content-Length', 0))
                post_data = self.rfile.read(content_length)
                
                # JSON 데이터 파싱
                data = json.loads(post_data.decode('utf-8'))
                
                # 성공 응답 데이터 생성
                response_data = {
                    "status": "success",
                    "received": data
                }
                response_body = json.dumps(response_data).encode('utf-8')
                
                # HTTP 상태 코드 200 및 헤더 설정
                self.send_response(200)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                
                # 응답 전송
                self.wfile.write(response_body)
                
            except (json.JSONDecodeError, UnicodeDecodeError):
                # JSON 파싱 실패 시 400 에러 반환
                self.send_error_response(400, "Invalid JSON")
        else:
            # 존재하지 않는 경로 요청 시 404 에러 반환
            self.send_error_response(404, "Not Found")

    def send_error_response(self, status_code, message):
        response_data = {"error": message}
        response_body = json.dumps(response_data).encode('utf-8')
        
        self.send_response(status_code)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(response_body)

def run(host="127.0.0.1", port=50012):
    server_address = (host, port)
    httpd = HTTPServer(server_address, MyRequestHandler)
    print(f"서버가 http://{host}:{port} 에서 실행 중입니다...")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n서버를 종료합니다.")
        httpd.server_close()

if __name__ == "__main__":
    run()

